#include <Arduino.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <atomic>
#include <cstdlib>
#include <driver/ledc.h>
#include <esp_sntp.h>
#include <esp_timer.h>
#include <sys/time.h>

#include "Board.h"
#include "Config.h"
#include "Dcf77.h"

namespace {
constexpr uint32_t kNtpIntervalMs = 60 * 60 * 1000;
constexpr uint32_t kMaxHoldoverSeconds = 6 * 60 * 60;
constexpr uint32_t kWifiRetryMs = 5000;
constexpr uint32_t kWifiPortalAfterMs = 60000;

WebServer webServer(80);
String ntpServer;
String hostname;
String portalName;
dcf77::Modulator modulator;  // Owned exclusively by the esp_timer task.
esp_timer_handle_t modulationTimer = nullptr;
std::atomic<bool> everSynced{false};
std::atomic<bool> transmitAllowed{true};
std::atomic<bool> transmitting{false};
std::atomic<bool> portalRequested{false};
std::atomic<uint32_t> lastSyncSeconds{0};
std::atomic<uint32_t> droppedFrames{0};
uint32_t restartAt = 0;

uint32_t monotonicSeconds() {
  return static_cast<uint32_t>(esp_timer_get_time() / 1000000);
}

bool clockUsable() {
  return everSynced.load() &&
         monotonicSeconds() - lastSyncSeconds.load() < kMaxHoldoverSeconds;
}

void timeSynchronized(timeval*) {
  lastSyncSeconds.store(monotonicSeconds());
  everSynced.store(true);
}

void modulationTick(void*) {
  timeval now{};
  gettimeofday(&now, nullptr);
  const int64_t monotonic = esp_timer_get_time();
  const int64_t utc = static_cast<int64_t>(now.tv_sec) * 1000000 + now.tv_usec;
  const bool usable = clockUsable() && now.tv_sec >= 1704067200 && transmitAllowed.load();
  const dcf77::Drive drive = modulator.update(utc, monotonic, usable);
  static dcf77::Drive previous = dcf77::Drive::Off;
  if (drive != previous) {
    const uint32_t duty = drive == dcf77::Drive::Full ? board::kFullDuty :
                          drive == dcf77::Drive::Reduced ? board::kReducedDuty : 0;
    // Only this task writes the LEDC channel. No Serial, Wi-Fi or NVS calls here.
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    previous = drive;
  }
  // Pulse with the second marks; slowly blink while waiting for usable time.
  const bool led = drive == dcf77::Drive::Reduced ||
                   (drive == dcf77::Drive::Off && transmitAllowed.load() &&
                    (monotonic / 500000) % 2 == 0);
  digitalWrite(board::kLed, led ? HIGH : LOW);
  transmitting.store(modulator.transmitting());
  droppedFrames.store(modulator.droppedFrames());
}

void startCarrier() {
  ledc_timer_config_t timer{};
  timer.speed_mode = LEDC_LOW_SPEED_MODE;  // The C3 has no high-speed LEDC mode.
  timer.duty_resolution = LEDC_TIMER_6_BIT;
  timer.timer_num = LEDC_TIMER_0;
  timer.freq_hz = board::kCarrierHz;
  timer.clk_cfg = LEDC_USE_APB_CLK;
  ESP_ERROR_CHECK(ledc_timer_config(&timer));
  ledc_channel_config_t channel{};
  channel.gpio_num = board::kAntenna;
  channel.speed_mode = LEDC_LOW_SPEED_MODE;
  channel.channel = LEDC_CHANNEL_0;
  channel.intr_type = LEDC_INTR_DISABLE;
  channel.timer_sel = LEDC_TIMER_0;
  channel.duty = 0;  // No RF until a real NTP sync and a complete minute boundary.
  channel.hpoint = 0;
  ESP_ERROR_CHECK(ledc_channel_config(&channel));
  Serial.printf("Carrier: %lu Hz, antenna GPIO%u, LED GPIO%u\n",
                static_cast<unsigned long>(ledc_get_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0)),
                board::kAntenna, board::kLed);
}

void startTiming() {
  sntp_set_time_sync_notification_cb(timeSynchronized);
  sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
  sntp_set_sync_interval(kNtpIntervalMs);
  // ntpServer remains allocated and unchanged for the lifetime of SNTP.
  configTzTime(dcf77::kTimezoneRule, ntpServer.c_str());
  esp_timer_create_args_t timer{};
  timer.callback = modulationTick;
  timer.dispatch_method = ESP_TIMER_TASK;
  timer.name = "dcf77";
  timer.skip_unhandled_events = true;
  ESP_ERROR_CHECK(esp_timer_create(&timer, &modulationTimer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(modulationTimer, 5000));
}

void loadSettings() {
  Preferences preferences;
  ntpServer = config::kDefaultNtpServer;
  if (preferences.begin("dcf77", true)) {
    ntpServer = preferences.getString("ntp", config::kDefaultNtpServer);
    preferences.end();
  }
  if (!config::validNtpHost(ntpServer.c_str())) ntpServer = config::kDefaultNtpServer;
}

bool saveSettings(const String& host) {
  Preferences preferences;
  if (!preferences.begin("dcf77", false)) return false;
  // One NVS value: updates cannot leave a partially saved configuration.
  const size_t bytes = preferences.putString("ntp", host);
  preferences.end();
  return bytes == host.length();
}

String htmlEscape(const String& value) {
  String result;
  result.reserve(value.length() + 16);
  for (size_t i = 0; i < value.length(); ++i) {
    switch (value[i]) {
      case '&': result += F("&amp;"); break;
      case '<': result += F("&lt;"); break;
      case '>': result += F("&gt;"); break;
      case '"': result += F("&quot;"); break;
      case '\'': result += F("&#39;"); break;
      default: result += value[i];
    }
  }
  return result;
}

String localTimeText() {
  if (!everSynced.load()) return "Waiting for NTP";
  const time_t now = time(nullptr);
  tm local{};
  localtime_r(&now, &local);
  char text[40];
  strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S %Z", &local);
  return String(text);
}

const char* transmissionStatus() {
  if (!transmitAllowed.load()) return "Restarting";
  if (!everSynced.load()) return "Waiting for NTP synchronization";
  if (!clockUsable()) return "Paused: NTP synchronization is more than 6 hours old";
  if (transmitting.load()) return "Transmitting";
  return "Waiting for the next complete minute";
}

void noCache() { webServer.sendHeader("Cache-Control", "no-store"); }

void showConfigPage(const String& message = "", int status = 200) {
  String page;
  page.reserve(5500);
  page = F("<!doctype html><html lang='en'><head><meta charset='utf-8'>"
           "<meta name='viewport' content='width=device-width,initial-scale=1'>"
           "<title>DCF77 transmitter</title><style>"
           "body{font:16px system-ui,sans-serif;max-width:40rem;margin:3rem auto;padding:0 1.2rem;background:#10232d;color:#eff6f8}"
           "h1{margin-bottom:.4rem}h2{font-size:1.1rem}section{padding:1.4rem;background:#1c3542;border-radius:12px;margin:1.2rem 0}"
           "label{display:block;margin:.9rem 0 .4rem}input{box-sizing:border-box;width:100%;font:inherit;padding:.7rem;border-radius:5px;border:1px solid #a5bec9}"
           "button{font:inherit;font-weight:600;padding:.7rem 1rem;border:0;border-radius:5px;background:#88d8d0;color:#112c35;cursor:pointer}"
           "p{line-height:1.5}.muted,small{color:#bbd1db}.time{font-size:1.5rem;font-variant-numeric:tabular-nums}.message{padding:1rem;background:#654530}"
           "</style></head><body><h1>DCF77 transmitter</h1><p class='muted'>Pacific time, automatically.</p>");
  if (message.length()) page += "<p class='message' role='alert'>" + htmlEscape(message) + "</p>";
  page += "<section><h2>America/Los_Angeles</h2><p id='time' class='time'>" + localTimeText() +
          "</p><p id='state'>" + transmissionStatus() + "</p>";
  page += F("<p class='muted'>PST in winter &middot; PDT in summer<br>Daylight-saving changes happen automatically.</p>");
  page += "<p><small>IP: " + WiFi.localIP().toString() + " &middot; Antenna: GPIO4 &middot; LED: GPIO5</small></p></section>";
  page += F("<section><h2>Time source</h2><form method='post' action='/save'>"
            "<label for='ntp'>NTP server</label><input id='ntp' name='ntp' required maxlength='253' autocapitalize='none' spellcheck='false' value='");
  page += htmlEscape(ntpServer);
  page += F("'><p class='muted'>A hostname or IPv4 address, such as pool.ntp.org or 192.168.1.10.</p>"
            "<button type='submit'>Save and restart</button></form></section>"
            "<section><h2>Wi-Fi</h2><p>Use the setup portal to change the Wi-Fi network.</p>"
            "<form method='post' action='/wifi'><button type='submit'>Open Wi-Fi setup</button></form></section>"
            "<p class='muted'>The signal carries Pacific wall-clock time. Leave any extra timezone adjustment on your receiving clock at zero.</p>");
  page += "<p><small>Firmware " FIRMWARE_VERSION " &middot; " + hostname + ".local</small></p>";
  page += F("<script>async function refresh(){try{const r=await fetch('/status',{cache:'no-store'});if(!r.ok)return;const s=await r.json();"
            "document.getElementById('time').textContent=s.localTime;document.getElementById('state').textContent=s.state;}catch(e){}}"
            "setInterval(refresh,5000);</script></body></html>");
  noCache();
  webServer.send(status, "text/html; charset=utf-8", page);
}

void saveConfigPage() {
  if (restartAt) { webServer.send(409, "text/plain", "Restart already scheduled"); return; }
  String candidate = webServer.arg("ntp");
  candidate.trim();
  if (!config::validNtpHost(candidate.c_str())) {
    showConfigPage("Enter a valid NTP hostname or IPv4 address without a URL, port or path.", 400);
    return;
  }
  // Silence the RF before flash writes, which can temporarily stall the CPU.
  transmitAllowed.store(false);
  delay(20);
  if (!saveSettings(candidate)) {
    transmitAllowed.store(true);
    showConfigPage("Could not save settings to flash. Please try again.", 500);
    return;
  }
  noCache();
  webServer.send(200, "text/html; charset=utf-8",
                 "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
                 "<meta http-equiv='refresh' content='12;url=/'><title>Settings saved</title></head>"
                 "<body><h1>Settings saved</h1><p>Restarting. This page will return in 12 seconds.</p></body></html>");
  restartAt = millis() + 500;
}

void configureRoutes() {
  webServer.on("/", HTTP_GET, [] { showConfigPage(); });
  webServer.on("/save", HTTP_POST, saveConfigPage);
  webServer.on("/status", HTTP_GET, [] {
    String json = "{\"localTime\":\"" + localTimeText() + "\",\"state\":\"" + transmissionStatus() +
                  "\",\"timezone\":\"America/Los_Angeles\",\"transmitting\":" +
                  (transmitting.load() ? "true" : "false") + ",\"lastSyncAgeSeconds\":";
    json += everSynced.load() ? String(monotonicSeconds() - lastSyncSeconds.load()) : String(-1);
    json += ",\"droppedFrames\":" + String(droppedFrames.load()) + "}";
    noCache();
    webServer.send(200, "application/json", json);
  });
  webServer.on("/wifi", HTTP_POST, [] {
    noCache();
    webServer.send(200, "text/html; charset=utf-8",
                   "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
                   "<title>Wi-Fi setup</title></head><body><h1>Wi-Fi setup</h1><p>Join <strong>" +
                   portalName + "</strong>, then open <a href='http://192.168.4.1/'>192.168.4.1</a>. "
                   "Return to the device's LAN address after saving.</p></body></html>");
    portalRequested.store(true);
  });
  webServer.onNotFound([] { webServer.sendHeader("Location", "/"); webServer.send(302); });
}

void startConfigServer() {
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  MDNS.end();
  if (MDNS.begin(hostname.c_str())) MDNS.addService("http", "tcp", 80);
  webServer.begin();
  Serial.printf("Configuration: http://%s/ or http://%s.local/\n",
                WiFi.localIP().toString().c_str(), hostname.c_str());
}

bool runPortal(bool force) {
  WiFiManager manager;
  manager.setDebugOutput(false);
  manager.setHostname(hostname);
  manager.setConnectTimeout(20);
  manager.setConfigPortalTimeout(300);
  manager.setAPCallback([](WiFiManager*) {
    Serial.printf("Wi-Fi setup: join %s, then open http://192.168.4.1/\n", portalName.c_str());
  });
  return force ? manager.startConfigPortal(portalName.c_str())
               : manager.autoConnect(portalName.c_str());
}

void monitorBootButton(void*) {
  uint32_t pressedAt = 0;
  bool handled = false;
  for (;;) {
    if (digitalRead(board::kBoot) == LOW) {
      if (!pressedAt) pressedAt = millis();
      if (!handled && millis() - pressedAt >= board::kBootHoldMs) {
        portalRequested.store(true);
        handled = true;
      }
    } else {
      pressedAt = 0;
      handled = false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(board::kAntenna, OUTPUT);
  digitalWrite(board::kAntenna, LOW);
  pinMode(board::kLed, OUTPUT);
  digitalWrite(board::kLed, HIGH);  // Solid during initial Wi-Fi provisioning.
  pinMode(board::kBoot, INPUT_PULLUP);
  const String chipId = String(static_cast<uint32_t>(ESP.getEfuseMac() & 0xffffff), HEX);
  hostname = "dcf77-" + chipId;
  portalName = "DCF77-Setup-" + chipId;
  loadSettings();
  startCarrier();
  if (xTaskCreate(monitorBootButton, "boot-button", 2048, nullptr, 1, nullptr) != pdPASS) {
    Serial.println("Cannot start BOOT button monitor");
    abort();
  }
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname.c_str());
  WiFi.setSleep(false);
  while (!runPortal(portalRequested.exchange(false))) {
    Serial.println("Wi-Fi setup timed out; retrying");
    delay(1000);
  }
  startTiming();
  configureRoutes();
  startConfigServer();
  Serial.printf("NTP: %s; timezone: %s; waiting for synchronization\n",
                ntpServer.c_str(), dcf77::kTimezoneName);
}

void loop() {
  static uint32_t disconnectedAt = 0;
  static uint32_t lastRetry = 0;
  if (restartAt && static_cast<int32_t>(millis() - restartAt) >= 0) ESP.restart();
  webServer.handleClient();
  const uint32_t now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    if (!disconnectedAt) disconnectedAt = now;
    if (now - lastRetry >= kWifiRetryMs) {
      lastRetry = now;
      WiFi.reconnect();
    }
    if (now - disconnectedAt >= kWifiPortalAfterMs) portalRequested.store(true);
  } else if (disconnectedAt) {
    disconnectedAt = 0;
    startConfigServer();
    sntp_restart();
  }
  if (!restartAt && portalRequested.exchange(false)) {
    // Give the HTTP acknowledgement time to leave before replacing the server.
    delay(200);
    webServer.stop();
    MDNS.end();
    Serial.println("Opening Wi-Fi setup portal");
    runPortal(true);
    portalRequested.store(false);
    disconnectedAt = 0;
    startConfigServer();
    if (WiFi.status() == WL_CONNECTED) sntp_restart();
  }
  delay(2);
}
