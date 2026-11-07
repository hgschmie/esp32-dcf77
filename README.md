# DCF77 transmitter for ESP32-C3 SuperMini

A PlatformIO firmware derived from [SensorsIot's DCF77 transmitter](https://github.com/SensorsIot/DCF77-Transmitter-for-ESP32), with captive Wi-Fi setup, a web configuration page, and automatic **America/Los_Angeles** time. It sends Pacific wall-clock time: PST (UTC−8) in winter and PDT (UTC−7) in summer. No manual offset is needed.

I bought a [cheap DCF-77 receiver module](https://www.amazon.com/dp/B0H6MHKKTF) and used its antenna for the transmitter (In fact I bought two of them so that I could use the second as a receiver to test the transmitter). This was much easier than trying to make an antenna myself.

## Pins and wiring

**Use GPIO4 for the antenna, not the original sketch's GPIO15.** GPIO15 belongs to the C3's flash interface and is not brought out on the usual SuperMini headers. GPIO4 and GPIO5 are available header outputs. They are neither boot strapping pins nor the native USB pins. The board's built-in LED is GPIO8; GPIO5 below is for your external LED.

| Function | Connection |
| --- | --- |
| Antenna output | GPIO4 → 1 kΩ series resistor → tuned ferrite antenna/loop → GND |
| External indicator LED | GPIO5 → 330 Ω–1 kΩ resistor → LED anode; LED cathode → GND |
| Setup button | Onboard BOOT button, GPIO9 |
| Power, upload, serial | USB-C, using a data-capable cable |

GPIO numbers refer to the labels on the board, not physical header positions. Pins are defined in [include/Board.h](include/Board.h). Keep the antenna resistor; the output is a 3.3 V GPIO driver. Reception depends on the antenna's tuning, orientation, distance and the receiving clock.

References: [Espressif C3 GPIO restrictions](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/peripherals/gpio.html), [Espressif's SuperMini variant pin definitions](https://github.com/espressif/arduino-esp32/blob/master/variants/nologo_esp32c3_super_mini/pins_arduino.h).

## Build and upload

Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html), or use its VS Code extension. Run from this directory:

```sh
pio run -e esp32-c3-supermini
pio run -e esp32-c3-supermini -t upload
pio device monitor -b 115200
```

Equivalent Make targets are `make build`, `make upload` and `make monitor`. The board profile is `esp32-c3-devkitm-1`, with 4 MB flash, DIO mode and native USB CDC enabled, matching the SuperMini configuration in the Factorio firmware example.

If automatic port selection fails:

```sh
pio device list
pio run -e esp32-c3-supermini -t upload --upload-port /dev/cu.usbmodemXXXX
pio device monitor --port /dev/cu.usbmodemXXXX -b 115200
```

For a board that will not enter the bootloader automatically, hold BOOT, press and release RESET, release BOOT, then upload. Press RESET after the upload if it remains in download mode.

The application binary is `.pio/build/esp32-c3-supermini/firmware.bin`. Use the PlatformIO upload command to install it together with the bootloader and partition table; the application binary alone is not a full-flash image.

Dependencies are pinned in [platformio.ini](platformio.ini): Espressif32 platform 6.10.0 (Arduino-ESP32 2.0.17) and WiFiManager 2.0.17. Build tools and packages stay in this project's ignored `.pio-core/` directory. No credentials are compiled into the source. The first build requires internet access.

On Apple Silicon, the registry's GCC 8.4 package contains an Intel executable. The pre-build script automatically installs **Espressif's native arm64 build of the same GCC release**, checks its published SHA-256, and changes only the project-local toolchain. It needs a maintained Python release with `tarfile`'s `data` extraction filter (Python 3.12+ recommended). Other host systems use PlatformIO's normal package. Details are in [scripts/ensure_native_toolchain.py](scripts/ensure_native_toolchain.py).

## First-time setup

1. Power the board and join **`DCF77-Setup-<chip-id>`** on your phone or computer.
2. The captive portal should open automatically. If it does not, visit **http://192.168.4.1/**.
3. Choose your 2.4 GHz Wi-Fi network and save its password.
4. Rejoin that network and open **`http://dcf77-<chip-id>.local/`**, or use the IP address printed on the serial console/router DHCP list.
5. Leave the NTP server at `pool.ntp.org`, or enter your server's bare hostname or IPv4 address and choose **Save and restart**.

The portal handles Wi-Fi; the normal web page handles NTP settings and shows the current Pacific time and transmitter status. A failed NTP lookup leaves the configuration page available. Settings survive resets and normal firmware uploads. NTP uses UDP port 123; the server field does not accept a URL, path, port or IPv6 address.

The firmware waits for a successful NTP response, then begins at the next complete minute boundary. Allow your receiving clock several complete telegrams to acquire the signal.

To change Wi-Fi later, choose **Open Wi-Fi setup** on the web page, or hold BOOT for five seconds **while the firmware is running**. Holding it during reset selects the bootloader instead. Setup sessions last five minutes; on first boot they repeat until Wi-Fi connects. The portal access point is open, and the HTTP page has no authentication/TLS; use them on a trusted local network.

## Automatic Pacific time

The device clock and NTP stay in UTC. The encoder converts the next minute to Pacific time using:

```text
America/Los_Angeles
PST8PDT,M3.2.0/2,M11.1.0/2
```

These are the current recurring US rules: advance at 02:00 on the second Sunday of March, and fall back at 02:00 on the first Sunday of November. The page displays `PST` or `PDT`. Rules are built into the firmware, so a future change to daylight-saving law would require a firmware update; NTP does not supply timezone rules. See the [POSIX timezone mapping](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv).

**This is a local DCF77-format time source carrying Pacific time.** Official DCF77 carries German time. DCF77 has standard/summer-time flags but no field naming America/Los_Angeles: this firmware sets those flags according to PST/PDT and announces the transition during the preceding hour. Receivers that display the decoded wall-clock fields directly should show Pacific time. Leave their extra timezone correction at zero. Receivers that insist on interpreting DCF77 as CET/CEST and converting it internally may need a different setting or may be unsuitable for this use.

## Signal and timing

- The C3's low-speed LEDC hardware generates the nominal 77.5 kHz carrier on GPIO4. Explicit 80 MHz APB clocking and 6-bit duty resolution give a nominal quantized frequency of about 77,500.6 Hz, before crystal error.
- Carrier duty is normally 32/64 (50%). At the start of seconds 0–58 it drops to 3/64 for 100 ms for a zero, or 200 ms for a one. Second 59 has no reduction: this is the minute marker.
- With a tuned antenna filtering out harmonics, the lower duty gives approximately `sin(π × 3/64) ≈ 14.7%` of the full-duty fundamental. This is duty-based carrier attenuation, not a reduction of the GPIO's 3.3 V peak voltage. Actual RF amplitude and receiver compatibility need checking with your antenna. It does not require a second GPIO, but it is not a calibrated analog AM output.
- The frame carries the **following minute's** complete date and time, including parity, weekday, DST flags and rollover across midnight/month/year. Leap-second insertion, weather data and DCF77 phase modulation are not implemented.
- A separate `esp_timer` task checks UTC phase every 5 ms; the web server and Wi-Fi portal run outside that task. No network requests, flash writes or serial printing run in the timing callback. Nominal envelope resolution is 5 ms, not laboratory time-transfer precision.
- A timer stall over 25 ms or wall-clock step over 50 ms abandons the current telegram and waits for a new minute. Settings saves silence the carrier before writing flash and restarting.
- SNTP refreshes hourly, using smooth corrections where possible. After initial synchronization, time can continue during an outage for up to six hours; then the carrier is silenced until another sync arrives.
- Wi-Fi reconnects automatically. After a minute offline, the setup portal opens; already synchronized timing can continue independently. Flash writes during Wi-Fi provisioning can cause a telegram to be abandoned.

The upstream sketch used complete carrier suppression during second marks. This implementation retains a reduced fundamental, using the C3's supported LEDC peripheral. Its RF behavior still needs hardware validation. Sources: [Espressif C3 LEDC documentation](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32c3/api-reference/peripherals/ledc.html), [PTB's DCF77 overview](https://www.ptb.de/cms/fileadmin/internet/publikationen/broschueren/About_Time_2019en.pdf).

## LED and status

| External GPIO5 LED | Meaning |
| --- | --- |
| Solid after power-up | Initial Wi-Fi connection/setup |
| Slow blink | Waiting for NTP or a fresh minute boundary; carrier off |
| Short 100/200 ms pulses | Transmitting second marks |
| No pulse during second 59 | Normal minute marker |

`GET /status` returns JSON containing local time, timezone, transmission state, last NTP sync age (`-1` before the first sync), and the count of frames abandoned after timing disruptions. The web page refreshes its time and status every five seconds without replacing an in-progress form edit.

## Verification

```sh
pio test -e native
```

Fourteen host tests cover a known BCD frame, parity, next-minute encoding, year/leap-day rollovers, weekday numbering, PST/PDT offsets and both DST transitions, the one-hour change announcement, startup synchronization, pulse lengths and the missing second, timing jitter, stalls/clock steps, clock expiry and NTP host validation.

The ESP32-C3 build and all 14 native tests passed on Apple Silicon. Build size: 919,646 bytes flash and 42,132 bytes static RAM. No board was connected during development; USB upload, captive-portal behavior on devices, RF output and clock reception have not been verified on hardware.

For the first hardware check, confirm GPIO4's carrier and 100/200 ms envelope with an oscilloscope, confirm the uninterrupted carrier at second 59, then try a receiving clock close to the tuned antenna. Use `/status` to check for dropped telegrams while loading the web page. If NTP never synchronizes, check the configured hostname, DNS and UDP/123 reachability; the web page remains usable.

