# Firmware architecture and implementation

This document describes the current firmware in `esp32/`. It replaces the
original implementation task, whose planned omissions (sound, motion sensing,
and time scheduling) no longer describe the code. PlatformIO builds the current
ESP32-C3 firmware from `esp32/src/`; the small sketch under
`testing/testingFilmware/` is a separate legacy hardware check.

## Runtime flow

- `src/main.cpp` owns the Arduino `setup()` and `loop()` and delegates to `Robot`.
- `Robot` coordinates startup diagnostics, time, console, sensor events, behavior,
  sound, and display updates.
- `BehaviourProgram` is portable C++ shared with the SDL simulator. It owns the
  persistent IDLE/TIRED/SLEEP state, schedule, interaction sequences, priorities,
  activity timers, and sound requests.
- `AnimationOutput` and `SoundOutput` are behavior-facing interfaces. Firmware
  implements them in `OutputDisplay` and `SoundManager`; the simulator provides
  its own implementations.
- `OutputDisplay` renders the active frame to the SH1106. The time is used by
  behavior and logs but is not drawn on the robot's OLED. Wi-Fi success/failure
  is shown briefly as a separate overlay.

## Source map

| Path | Responsibility |
| --- | --- |
| `esp32/src/Robot.h/.cpp` | Top-level orchestration, clock selection, status, and event routing |
| `esp32/src/behavior/BehaviourProgram.h/.cpp` | Schedule, reactions, priorities, activity selection, and return timers |
| `esp32/src/input/InputController.h/.cpp` | TTP223 interrupt edge queue, debounce, TAP/HOLD/RELEASE, motion events |
| `esp32/src/input/MotionSensor.h/.cpp` | MPU6050 sampling and shake detection |
| `esp32/src/display/OutputDisplay.h/.cpp` | OLED initialization, framebuffer transfer, clock-free face and Wi-Fi overlay |
| `esp32/src/display/animations/` | Firmware animation catalog and individual animation definitions |
| `esp32/src/sound/SoundData.cpp` | Shared frequency/duration score catalog |
| `esp32/src/sound/SoundManager.h/.cpp` | Nonblocking score playback through the platform tone output |
| `esp32/src/time/TimeManager.h/.cpp` | Wi-Fi multi-network selection, SNTP verification, retry, and local time |
| `esp32/src/debug/DebugConsole.h/.cpp` | Development USB Serial command parser |
| `esp32/src/diagnostics/HardwareDiagnostics.cpp` | Startup checks and fatal OLED handling |
| `esp32/src/config/` | Pin assignments and build-time configuration |
| `esp32/include/WifiConfig.example.h` | Example multi-network credential list |
| `esp32/host/` | Host stubs and tests for firmware code |
| `testing/simulator/` | SDL2 display, behavior controls, and animation previews |

## Hardware and input

Default pins and timing are in `esp32/src/config/Pins.h`: OLED I2C SDA GPIO4,
SCL GPIO5, TTP223 output GPIO1, and tone output GPIO3. Touch is active HIGH,
debounce is 35 ms, and HOLD begins after 700 ms. The GPIO3 signal is intended to
drive a suitable piezo driver; electrical compatibility and acoustic output
must be checked with the actual circuit.

The MPU6050 uses I2C address 0x68. The driver samples every 10 ms, removes the
gravity baseline, and emits a shake after repeated acceleration reversals cross
its default 0.070 g threshold. A single bump is not enough. If the sensor is
absent or repeatedly fails to read, shake input is disabled; OLED/touch behavior
continues. The current `MotionSensitivity` setter is available but normal
firmware uses the default threshold.

The touch ISR timestamps and queues edges so display transfers do not lose a
short press. The main loop debounces in order and emits one TAP on release, or
HOLD while still pressed followed by RELEASE. Interrupt classification during
startup is discarded until the power-on animation finishes.

## Behavior and rendering

The schedule uses local time: IDLE from 07:00, TIRED from 21:00, and SLEEP from
21:30. Without valid time the robot remains in IDLE and the schedule is inactive.
TAP and HOLD use state-specific wake/return sequences; shaking preserves the
persistent state and ends in a dizzy recovery. Touch, scheduled transitions,
shake recovery, spontaneous activity, and blink playback follow the priorities
implemented by `BehaviourProgram`.

Animation frames are timed from elapsed milliseconds, not frame counts. The
firmware builds active animations in a compact static buffer; transient pose
data is captured before another animation reuses that buffer. The simulator
shares behavior and the approved character-animation data but uses its own SDL
framebuffer and audio backend.

## Time and Wi-Fi

The private `esp32/include/WifiConfig.h` contains `WifiConfig::NETWORKS`, an array
of SSID/password pairs. `WiFiMulti` scans known networks and chooses an available
candidate, then the ESP32 associates with one network at a time. After fresh
NTP synchronization, SNTP is stopped and the Wi-Fi radio is turned off. The
system clock continues offline; every reboot requires a fresh sync unless a
manual source is selected during that run. See [Wi-Fi and time setup](WifiTime.md).

`time HH:MM` selects an independent simulated clock. `TIME <local epoch seconds>`
sets a manual fallback clock and cancels automatic synchronization until reboot.
`time auto` returns to the real source, or to UNSET if none is available. These
commands do not write time or credentials to persistent storage.

## Diagnostics and known limits

Startup checks the I2C bus and OLED ACK before and after U8g2 initialization.
An OLED failure latches ERROR until restart. The TTP223 is reported as configured
because its single digital output cannot prove electrical health. MPU6050 and
sound initialization failures are reported and disable only those components.
An OLED ACK or successful initialization cannot verify visible pixels.

The development environment enables the USB Serial console at 115200 baud. The
`esp32c3-production` environment compiles it out. Current wiring, console
commands, and interaction details are documented in the
[firmware guide](../esp32/README.md) and [debug console guide](DebugConsole.md).

```powershell
pio run -d esp32 -e esp32c3
pio run -d esp32 -e esp32c3-production
```

Host-side tests are available through `./esp32/host/run.ps1`; this document does
not claim that a current firmware build or physical hardware test was run.
