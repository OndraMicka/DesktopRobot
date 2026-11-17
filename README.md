# DesktopRobot

DesktopRobot is an ESP32-C3 desktop companion with a 128×64 monochrome OLED,
capacitive touch input, MPU6050 motion detection, and tone output. The repository
contains the production firmware, an SDL2 simulator, a separate minimal hardware
test sketch, host-side firmware tests, and project documentation.

## Current capabilities

- Animated IDLE, TIRED, and SLEEP behavior with local-time schedule boundaries
  at 07:00, 21:00, and 21:30.
- Touch TAP, HOLD, and RELEASE reactions, plus MPU6050 shake detection.
- Tone sequences through LEDC on GPIO3. The electrical driver/speaker connection
  still needs physical verification.
- Optional Wi-Fi/NTP time synchronization from multiple saved 2.4 GHz networks.
  The time controls the behavior schedule; the OLED does not show a clock.
- Startup checks for the OLED and motion sensor. Missing OLED is a latched error;
  missing MPU6050 disables shake input while the robot continues.

## Repository map

| Path | Contents |
| --- | --- |
| `esp32/` | PlatformIO firmware, behavior, display, input, sound, and time services |
| `esp32/include/WifiConfig.example.h` | Public template for private multi-network credentials |
| `esp32/host/` | Host stubs and firmware behavior/time/sound tests |
| `testing/simulator/` | SDL2 simulator, animation previews, and simulator tests |
| `testing/testingFilmware/` | Standalone early OLED/touch hardware test firmware |
| `docs/` | Behavior specification, setup guides, wiring diagram, and model assets |

## Build and run

Build the simulator on Windows with MSYS2 UCRT64 and SDL2:

```powershell
./testing/simulator/build.ps1
./testing/build/desktop_robot_simulator.exe
```

Build or upload the current ESP32-C3 firmware with PlatformIO:

```powershell
pio run -d esp32 -e esp32c3
pio run -d esp32 -e esp32c3 -t upload
pio device monitor -d esp32 -b 115200
```

For Wi-Fi time setup, wiring, firmware architecture, and console commands, see
the [firmware guide](esp32/README.md), [Wi-Fi and time guide](docs/WifiTime.md),
and [debug console guide](docs/DebugConsole.md). The old standalone hardware test
is built separately with `pio run -d testing/testingFilmware -e esp32c3`;
the production firmware is under `esp32/`.

The behavior specification describes the intended behavior in detail. The
firmware guide records implementation choices and known hardware limitations.
