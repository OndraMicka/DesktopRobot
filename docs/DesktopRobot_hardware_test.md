# Standalone OLED and touch test

`testing/testingFilmware/` is an early, standalone PlatformIO sketch for checking
the ESP32-C3, SH1106 display, and TTP223 wiring. It is separate from the current
modular firmware in `esp32/`. The test does not include the MPU6050, Wi-Fi, daily
schedule, current robot animations, or sound output. For normal robot operation,
build and upload `esp32/` instead.

## Wiring

| Module | Pin | ESP32-C3 Super Mini |
| --- | --- | --- |
| SH1106 OLED | VCC / GND | 3V3 / GND |
| SH1106 OLED | SDA / SCL | GPIO4 / GPIO5 |
| TTP223 | VCC / GND | 3V3 / GND |
| TTP223 | OUT | GPIO1 |

Check the labels on the exact boards before connecting power. OLED address,
touch polarity, and pins are defined in the sketch. The standalone test expects
OLED address 0x3C or 0x3D and an active-HIGH touch output.

## Build and upload

From the repository root, use PlatformIO:

```powershell
pio run -d testing/testingFilmware -e esp32c3
pio run -d testing/testingFilmware -e esp32c3 -t upload
pio device monitor -d testing/testingFilmware -b 115200
```

The sketch scans I2C, initializes a detected OLED, and reports touch press,
TAP, and HOLD events. Its simplified face reactions only validate basic wiring;
they do not represent the behavior of the production firmware.

For current pin assignments and complete hardware diagnostics, see
[`esp32/src/config/Pins.h`](../esp32/src/config/Pins.h),
[`esp32/src/diagnostics/HardwareDiagnostics.cpp`](../esp32/src/diagnostics/HardwareDiagnostics.cpp),
and the [firmware guide](../esp32/README.md).
