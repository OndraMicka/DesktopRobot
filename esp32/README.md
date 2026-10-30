# DesktopRobot firmware and shared behavior

ESP32-C3 Super Mini, SH1106 128x64 and TTP223. The hardware test in
`testing/testingFilmware/` remains separate. The SDL simulator now uses the same
`BehaviourProgram` and animation definitions as firmware.

## Wiring and build

| Signal | ESP32-C3 |
|---|---|
| OLED SDA | GPIO4 |
| OLED SCL | GPIO5 |
| TTP223 OUT | GPIO1 |
| Module VCC / GND | 3V3 / GND |

Configuration: `src/config/Pins.h`, active HIGH, debounce 35 ms, HOLD 700 ms,
OLED address 0x3C (explicitly change to 0x3D for that panel).

```powershell
pio run -d esp32 -e esp32c3
pio run -d esp32 -e esp32c3 -t upload
pio device monitor -d esp32 -b 115200
./esp32/host/run.ps1
```

If needed, replace `pio` with
`& "$env:USERPROFILE/.platformio/penv/Scripts/platformio.exe"`.
Add `--upload-port COMx` for upload or `-p COMx` for monitor when needed.
Reset with the monitor open to capture diagnostics; boot does not wait for USB.
PlatformIO configuration is now standalone: inherited `extra_configs` used to
overwrite the firmware build flags. Board, platform specification, U8g2 2.36.15
and USB CDC flags are preserved; C++17 and ROBOT_COMPACT_ANIMATIONS are explicit.

## Responsibilities

- `main` calls Robot initialization and updates only.
- Robot runs startup checks, updates the clock, supplies input and renders output.
- `behavior/BehaviourProgram` is portable C++ without Arduino or SDL. It owns
  schedules, temporary states, activity selection, priorities and sequences.
- `AnimationOutput` is its small output port. Firmware OutputDisplay and the SDL
  adapter implement it; neither chooses the following behavior.
- The simulator AnimationPlayer provides explicit playback, retained final frames,
  bounded expression playback, interrupt/replace, loop timing and completion.
  Its previous auto-resume methods remain available for manual previews.
- AnimationCatalog dispatches to individual classes in `src/display/animations/`,
for example BlinkAnimation, AngryAnimation and SmileAnimation. Each class owns
its hardcoded sequence and timing; AnimationBuilder supplies common geometry
and interpolation. The simulator has its own TestAnimations.cpp implementation.
All firmware sources live inside `esp32/`.
Firmware generates only
  the active animation in one static buffer; switching invalidates earlier frame
  references. Recovery captures the release pose before overwriting that buffer.
  There are no large automatic animation-storage objects on the stack.
- InputController produces TAP, HOLD and RELEASE. A release after HOLD cannot
  generate TAP. Releases below HOLD are TAP, including the suggested 300-700 ms
  pending-classification range. A missed hold threshold still emits HOLD followed
  by RELEASE on the following update.

## Local clock on ESP32

No RTC, NTP or Wi-Fi dependency was added. At startup the clock is explicitly
UNSET and the robot stays IDLE with interaction/spontaneous behavior. The daily
schedule becomes active only when valid local time is supplied.

Send one Serial line `TIME <local epoch seconds>` (integer seconds, newline).
For testing a day count of zero is sufficient:

| Serial command | Local time |
|---|---|
| `TIME 25199` | 06:59:59 |
| `TIME 75599` | 20:59:59 |
| `TIME 77399` | 21:29:59 |
| `TIME 21600` | 06:00:00 |
| `TIME 79200` | 22:00:00 |

The clock advances using elapsed milliseconds and wraps across days. Software
can call `Robot::setLocalTime(day * 86400 + secondsSinceMidnight)`; a future RTC
or time-sync provider can use the same interface. It is local civil time, not a
UTC timestamp. Reconfigure after reboot or daylight-saving changes. The firmware
does not invent a valid time or claim to retain it without a time source.

## Behavior

Schedule boundaries are 07:00 IDLE, 21:00 TIRED, 21:30 SLEEP. All six state
transitions use existing catalog IDs. Morning adds the existing daytime yawn and
look-around after the opening transition. State indicators show the destination
as soon as a transition starts.

TAP: IDLE -> Angry; TIRED -> TiredToIdle -> Angry -> temporary IDLE;
SLEEP -> SleepToTired -> Angry -> temporary TIRED. Temporary IDLE expires after
five minutes into TIRED, followed by SLEEP after another five. Night wake-ups
return to SLEEP after five minutes. Wake-ups before 07:00 remain TIRED until the
morning boundary. Timers begin after the completed reaction/release.

HOLD: LookUp -> Smile, preceded by SleepToTired when sleeping. The existing
smiling frame is held until RELEASE, without replaying the animation. Release
restores IDLE/TIRED or leaves a woken sleeper TIRED under the temporary rules.
An early release lets the current friendly sequence finish once.

Priority: latched ERROR, then scheduled transitions, then shake/recovery and user
interactions, required transitions, spontaneous activity, blinking. A scheduled
boundary cancels the old interaction/sequence and its temporary return target;
release cannot restore the obsolete state. Scheduled sequences cannot be
interrupted by input. Duplicate SHAKE_START does not restart SHAKING; SHAKE_END
uses the exact current frame for DIZZY, which a fresh SHAKE_START can interrupt.
Shaking preserves the permanent state. Physical motion detection is not present;
software can inject events through `Robot::handleEvent` and the simulator.

`BehaviourConfig` contains per-state decision intervals, blink intervals,
activity weights/cooldowns, quiet weight, temporary duration and random seed.
Default decision intervals: IDLE 10-60 s, TIRED 20-90 s, SLEEP 30-120 s.
Blink intervals: IDLE 3-7 s, TIRED 6-14 s, no ordinary SLEEP blinking.
Blinking has a separate timer and does not postpone activity decisions.
Activities cannot interrupt an interaction. Selection excludes the last chosen
activity and activities on cooldown, and includes a quiet outcome. Random input
can be injected for deterministic tests; firmware uses esp_random.

## Specification reconciliation

The detailed behavior specification and current simulator definitions take
precedence over the earlier minimal-firmware implementation document.

- SURPRISED no longer exists. SleepToTired provides the gradual waking/opening
  before Angry; no replacement startled animation was invented.
- The current TIRED-to-SLEEP ID is TiredToSleep; its label is TIRED TO SLEEP,
  although an older specification table calls it GO TO SLEEP.
- Smile serves contented and self-satisfied expressions with combined weight 10.
  RandomCuriosity covers checking surroundings. Original supported IDLE weights
  (LookAround 35, Thinking 10, IdleYawn 8, Bored 5, Listening 7) are retained.
- There are no dedicated HUM or almost-falling-asleep animations. These are not
  synthesized or selected. Quiet outcomes provide idle periods. Unspecified
  intervals, other weights and cooldowns are configurable initial defaults.
- Buzzer hardware and a motion sensor are not connected. Firmware emits no sound
  and performs no physical shake detection. The SDL adapter uses existing Tap1
  and Hold1 interaction sounds. No new sound or hardware dependency was added.
- Fixed schedule boundaries immediately preempt lower-priority playback. This
  follows the priority table; keeping an interaction beyond a boundary is not
  used. Arbitrary transitions between incompatible existing expressions do not
  gain invented morphs. The existing exact shake-to-dizzy blend is preserved.

## Diagnostics

Every restart checks I2C and OLED ACK before/after U8g2 initialization. A critical
failure latches ERROR until reboot, disables inputs/normal behavior and displays
a static sad face with ERROR when the OLED is available. U8g2 initialization and
ACK cannot verify visible pixels. TTP223 is logged as CONFIGURED, not electrically
verified. Display disconnection during runtime is not continuously monitored.

## Simulator and tests

Build the SDL app using the existing CMake project (`cmake -S testing -B build`,
then `cmake --build build`); SDL2 development files are required. CTest includes
state-transition and shared-behavior tests (`ctest --test-dir build --output-on-failure`).
Windows without CMake can run all non-SDL tests using `./esp32/host/run.ps1`.

The simulator starts in automatic mode with computer local time. Its window
title shows the mode and time. Controls:

- A or AUTO / PREVIEW: switch between behavior and the original animation previews.
- T or TAP: tap. H or HOLD: hold; release the key/mouse or use RELEASE.
- Space or SHAKE - HOLD: shake; release to run recovery.
- Left/right arrow: shift the simulated local clock by five minutes.
- F7 / F8 / F9: set one second before 07:00 / 21:00 / 21:30.
- Existing state/animation buttons remain available. In automatic mode manual
  state selection does not erase the next schedule boundary.

Tests cover all schedule boundaries, six transitions, TAP/HOLD/RELEASE in each
state, early/night wake-ups, two-stage timeouts, interruption, schedule priority,
shake/recovery, deterministic random selection, cooldowns, quiet outcomes,
state restrictions, millis overflow, diagnostics and ERROR.
Catalog snapshots compare every pixel and frame duration of all 25 animations,
three state loops and recovery from every shake frame. The compact and desktop
catalog hashes match the original pre-change catalog.

ESP32 build, host tests and SDL2 compilation passed. Physical upload, touch/OLED
verification and interactive GUI checks were not performed.
