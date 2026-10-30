# DesktopRobot firmware

Modular firmware for ESP32-C3 Super Mini, SH1106 128x64 and TTP223.
The original hardware test remains in `testing/testingFilmware/`.

## Wiring

| Module | Signal | ESP32-C3 |
|---|---|---|
| OLED | SDA | GPIO4 |
| OLED | SCL | GPIO5 |
| TTP223 | OUT | GPIO1 |
| Both | VCC / GND | 3V3 / GND |

`src/config/Pins.h` preserves the previous hardware test pins. It also defines
active-HIGH touch polarity, 35 ms debounce, 1000 ms HOLD, and OLED address 0x3C.
Set `OLED_ADDRESS` explicitly to 0x3D for that module variant. No automatic pin
search or configuration changes occur.

## Build, upload and monitor

From the repository root:

```powershell
pio run -d esp32 -e esp32c3
pio run -d esp32 -e esp32c3 -t upload
pio device monitor -d esp32 -b 115200
```

Inside `esp32/`, use `pio run -e esp32c3`. If `pio` is not in PATH, substitute
`& "$env:USERPROFILE/.platformio/penv/Scripts/platformio.exe"` for `pio`.
With multiple connected devices, add `--upload-port COMx` to upload and `-p COMx`
to monitor. Open the monitor and reset the board to capture startup logs: boot
does not wait for a USB host.

The existing board/framework/U8g2 2.36.15 configuration is inherited from the
hardware test, with the same USB CDC flags and no requested platform upgrade.
The firmware uses its own source/build folders and C++17 for shared graphics.

## Files and data flow

- `src/main.cpp`: only initializes and updates Robot.
- `src/Robot.h/.cpp`: component ownership, startup and event delivery.
- `src/config/Pins.h`: fixed wiring and input settings.
- `src/input/InputController.h/.cpp`: debounce, TAP and HOLD; no graphics.
- `src/behavior/BehaviourProgram.h/.cpp`: the sole state machine and animation selection.
- `src/display/OutputDisplay.h/.cpp`: U8g2 output and static error screen.
- `src/display/AnimationPlayer.h/.cpp`: nonblocking frame timing and latched completion.
- `src/display/AnimationId.h`: animation identifiers.
- `src/display/SharedGraphics.h/.cpp`: compiles the existing framebuffer and
  DrawCommand renderer from `testing/simulator/src/` without SDL or copied primitives.
- `src/display/animations/AnimationCatalog.h/.cpp`, `Poses.h`,
  `BlinkAnimation.cpp`, `AngryAnimation.cpp`, `SmileAnimation.cpp`,
  `YawnAnimation.cpp`: constant frames and catalog.
- `src/diagnostics/HardwareDiagnostics.h/.cpp`: Serial, I2C, OLED ACK before/after
  initialization, and touch configuration.
- `host/tests.cpp`, `host/run.ps1`, `host/stubs/{Arduino,Wire,U8g2lib}.h`:
  automated tests with simulated hardware.

GPIO -> InputController -> TAP/HOLD -> BehaviourProgram -> AnimationId ->
OutputDisplay -> AnimationPlayer -> shared framebuffer -> U8g2.
The player reports completion; behavior chooses the following animation.

TAP on release plays ANGRY, HOLD once while pressed plays SMILE. Release after
HOLD emits no TAP. A new event interrupts any normal animation, including the
same expression. IDLE blinks after four seconds of quiet. TIRED and SLEEP are
available through explicit `setState`, with no automatic daily schedule.
ERROR is latched until restart and ignores input, keeping a static sad face and
ERROR text when the display is initialized. Frames are sent only when needed.

The PC animation catalog reserves large interpolation tables for all animations;
the firmware instead uses a small constant catalog based on the existing eye and
mouth geometry. The PC player also automatically resumes a prior loop; this
player retains the last frame and leaves the next selection to behavior.
The PC simulator and hardware test sources are unchanged.

## Diagnostics and limits

Diagnostics run on every boot. Missing OLED ACK or failed I2C/display initialization
enters ERROR without normal animations or touch reads. U8g2 `begin()` alone does
not verify electrical operation. ACK confirms a responding I2C device, not visible
pixels or the correct panel type. An unavailable OLED cannot display the error;
the reason is logged to Serial when connected.

TTP223 has only a digital OUT, so the log says `CONFIGURED (not electrically
verifiable)`, never VERIFIED. Boot does not wait for touch. This is a startup
check, not continuous detection of display disconnection during operation.
Input uses one event slot consumed by Robot on every update; other consumers
must also poll it regularly.

## Verification

Windows host tests require g++ in PATH:

```powershell
./esp32/host/run.ps1
```

Tests cover debounce, one TAP on release, HOLD during press without a later TAP,
the 1000 ms boundary, slow updates, millis wraparound, animation completion and
interruption, blinking, explicit states and failed boot without input processing.
These mocks do not replace electrical or visual tests.

1. Build and upload using the commands above. Open the monitor, reset the board,
   and verify OLED OK, TOUCH CONFIGURED and STATE IDLE.
2. Check the calm face and blink after four seconds.
3. Touch briefly and release: exactly one TAP, ANGRY, then IDLE.
4. Hold over one second: exactly one HOLD while still touching, SMILE, then IDLE.
   Release must not emit TAP.
5. Touch during ANGRY/SMILE: the new reaction interrupts playback.
6. Power off, disconnect OLED, and power on: OLED FAILED, reason, STATE ERROR.
   Touch must produce no events or normal animations. Power off, reconnect OLED,
   and boot again to verify diagnostics repeat on every restart.

Build passed for esp32c3: RAM 17524/327680 bytes, flash 364174/1310720 bytes.
Host tests passed. Upload and physical hardware tests were not performed.
