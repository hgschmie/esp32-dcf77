# Provenance and dependencies

The starting point is [SensorsIot/DCF77-Transmitter-for-ESP32](https://github.com/SensorsIot/DCF77-Transmitter-for-ESP32), revision [`037a3579e332ac21614810525843b956fd221502`](https://github.com/SensorsIot/DCF77-Transmitter-for-ESP32/tree/037a3579e332ac21614810525843b956fd221502).

The `CodeTime`, `Bin2Bcd` and `DcfOut` routines in `DFC77_ESP32/DFC77_ESP32.ino` supplied the BCD bit assignments, parity groups, next-minute framing, second-mark durations and LEDC carrier approach. The source header credits [aknik/ESP32](https://github.com/aknik/ESP32/blob/master/DFC77/DFC77_esp32_Solo.ino), G6EJD and DeltaZero. Those credits are retained here. The upstream README identifies its license as MIT; the inspected revision does not include a separate LICENSE file or full license notice.

This adaptation separates encoding from hardware timing, corrects full calendar rollover, uses C3-supported pins and low-speed LEDC, adds reduced-duty marks, keeps networking active, and adds Pacific DST conversion, provisioning, persisted NTP settings and host tests. It does not carry over the scheduled deep-sleep/cron logic or hardcoded Wi-Fi credentials.

The PlatformIO settings and provisioning/management architecture were also informed by the user's local `~/factorio/mods/lamp-led-display/firmware` example: Arduino framework, SuperMini board flags, WiFiManager, Preferences, WebServer, mDNS and a five-second BOOT-button hold.

Dependencies are downloaded by PlatformIO, with their own upstream license notices:

- [Arduino-ESP32](https://github.com/espressif/arduino-esp32), including Espressif's bundled SDK and built-in libraries.
- [WiFiManager 2.0.17](https://github.com/tzapu/WiFiManager/tree/v2.0.17), MIT.
- [Unity](https://github.com/ThrowTheSwitch/Unity), MIT, for host tests.
- [Espressif GCC toolchain](https://github.com/espressif/crosstool-NG/releases/tag/esp-2021r2-patch5), with the licenses and runtime exceptions distributed in the toolchain package.

The native macOS arm64 toolchain adaptation uses Espressif's [published release URL and SHA-256](https://docs.espressif.com/projects/esp-idf/en/v4.4.3/esp32/api-guides/tools/idf-tools.html), retaining the package contents and notices in the ignored local build cache.
