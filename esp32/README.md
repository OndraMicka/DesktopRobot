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
| Piezo driver control (3.3 V logic) | GPIO3 |
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

## Debug console and local clock

The development `esp32c3` environment has a nonblocking USB Serial console at
115200 baud. See [console commands and test scenarios](../docs/DebugConsole.md).
Build `esp32c3-production` to compile it out (`ROBOT_DEBUG_CONSOLE=0`).

`time HH:MM` selects a separate simulated clock running at normal speed.
`time auto` restores the independently advancing real clock and reconciles the
schedule, removing temporary debug reactions. Neither operation writes to NVS.

AUTO is provided by `TimeManager`: after hardware diagnostics, ESP32 connects
using `include/WifiConfig.h`, waits for a fresh verified NTP response, then stops
SNTP and turns Wi-Fi OFF. Failed attempts retry without blocking behavior.
The exact Czech timezone is CET-1CEST,M3.5.0/2,M10.5.0/3.
See [Wi-Fi setup and time verification](../docs/WifiTime.md).

Copy `include/WifiConfig.example.h` to the Git-ignored `include/WifiConfig.h`
if missing, then enter your SSID/password locally. Without credentials the
firmware builds but reports missing configuration and leaves time invalid.
The ESP32 communicates directly with Wi-Fi/NTP; the PC is not a time provider.

`Robot::setLocalTime` and legacy `TIME <local epoch seconds>` remain available.
Manual time cancels automatic synchronization until reboot, including in-flight
responses. Simulated HH:MM time is independent. `time auto` restores the real
NTP/manual source, or explicitly reports UNSET if none exists. Every restart
requires fresh NTP synchronization; manual/simulated time is not persisted.

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
An early release cancels the friendly sequence and restores its return state.

Priority: latched ERROR, then scheduled transitions, then shake/recovery and user
interactions, required transitions, then spontaneous activity and blinking by their deadlines. A scheduled
boundary cancels the old interaction/sequence and its temporary return target;
release cannot restore the obsolete state. Scheduled sequences cannot be
interrupted by input. Duplicate SHAKE_START does not restart SHAKING; SHAKE_END
uses the exact current frame for DIZZY, which a fresh SHAKE_START can interrupt.
Shaking preserves the permanent state. Physical motion detection is not present;
software can inject events through `Robot::handleEvent` and the simulator.

`BehaviourConfig` contains per-state decision intervals, blink intervals,
activity weights/cooldowns, quiet weight, temporary duration and random seed.
Default decision intervals: IDLE 6-14 s, TIRED 8-18 s, SLEEP 12-25 s.
The quiet outcome has weight 3. Cooldowns: directional looks 5 s, LookAround
8 s, Thinking 10 s, Listening/Smile/Bored 20 s, RandomCuriosity 15 s,
IdleYawn 15 s, TiredYawn 12 s, SlowEyeMovement 12 s, SleepZ 10 s,
SleepEyeOpen 20 s. RandomCuriosity has weight 15, Bored 8 and SleepZ 10.
Animation frame timing is unchanged; the previous spontaneous
activity remains excluded from the next draw.
Blink intervals: IDLE 2-5 s, TIRED 4-9 s, no ordinary SLEEP blinking.
Blinking has a separate timer. Activity completions and quiet draws cannot reset
it. A random 0.8-2.2 s breathing gap follows each activity or blink. When both
deadlines are due, the older one wins; exact ties are randomized. Completing a
blink preserves the activity deadline. Neither kind is chained immediately to
the other, and multiple ordinary blinks may occur between activities.
Activities cannot interrupt an interaction. Selection excludes the last chosen
activity and activities on cooldown, and includes a quiet outcome. Random input
can be injected for deterministic tests; firmware uses esp_random.

The ten reviewed character animations are now part of the firmware catalog and
automatic activity selection. Their frames are immutable flash data in
`CharacterAnimationData.inc`, exported from the approved simulator previews;
they do not allocate a second animation workspace in RAM.

| State | Activity | Weight | Cooldown between starts |
| --- | --- | ---: | ---: |
| IDLE | Hmm | 12 | 15 s |
| IDLE | Kiss | 5 | 45 s |
| IDLE | StartleLeft | 7 | 30 s |
| IDLE | StartleUpRight | 7 | 30 s |
| TIRED | Microsleep | 18 | 20 s |
| TIRED | Sigh | 20 | 15 s |
| TIRED | HeavyLids | 28 | 12 s |
| TIRED | SleepyNod | 22 | 15 s |
| SLEEP | SleepRoll | 12 | 25 s |
| SLEEP | SleepTwitch | 10 | 18 s |

Both startle directions also share a 20-second cooldown. Weights normalize over
currently eligible activities plus the quiet outcome; they are not fixed
percentages. Relative activity weights are unchanged; cooldowns above control repeat frequency.
The simulator's AUTO mode uses this same behavior configuration.

## Specification reconciliation

The detailed behavior specification and current simulator definitions take
precedence over the earlier minimal-firmware implementation document.

- SURPRISED no longer exists. SleepToTired provides the gradual waking/opening
  before Angry; no replacement startled animation was invented.
- The current TIRED-to-SLEEP ID is TiredToSleep; its label is TIRED TO SLEEP,
  although an older specification table calls it GO TO SLEEP.
- Smile serves contented and self-satisfied expressions with combined weight 10.
  RandomCuriosity covers checking surroundings with weight 15. Other IDLE weights
  include LookAround 35, Thinking 10, IdleYawn 8, Bored 8 and Listening 7.
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

On every successful boot/reset, the OLED is cleared before any face is shown.
The shared POWER ON animation runs for 4.35 seconds: 900 ms blank, 1 second
unfolding the eyes sideways into lines, then 2 seconds opening them vertically.
The mouth grows during the first 400 ms of that vertical opening, followed by
a 450 ms hold. Touch, shake, state commands and schedule changes cannot replace
the startup frames. Clocks and status commands remain active; after startup the
current time determines the normal state. Hardware errors retain priority.

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
Catalog snapshots compare every pixel and frame duration of all 35 animations,
three state loops and recovery from every shake frame. The compact and desktop
catalog hashes match the original pre-change catalog.

ESP32 build, host tests and SDL2 compilation passed. Physical upload, touch/OLED
verification and interactive GUI checks were not performed.

## Touch reactions

Touch input is sampled before automatic behavior. From the first sampled press
through TAP/HOLD classification and release debounce, spontaneous activities
and blinking are blocked and any current spontaneous animation is interrupted.
TAP/HOLD and SHAKE_START interrupt spontaneous playback immediately; random
animations cannot replace a reaction, shaking or its recovery.
GPIO edges are timestamped in an interrupt and buffered while OLED transfers
block the loop. Debounce replays those edges in order, preserving complete short
taps and consecutive TAP/HOLD/RELEASE events instead of overwriting them. There
is no cooldown after a reaction: a new TAP immediately restarts Angry, including
when it arrives during the previous reaction or just after its last frame.

TAP is unchanged: IDLE plays Angry; TIRED and SLEEP first use the quick
TouchWakeAnimation (220 ms), then Angry.
HOLD in IDLE uses the original complete slow LookUp animation from e88c93c,
then Smile. TIRED and SLEEP use dedicated TouchLookUpAnimation variants:
slowly open the half-closed/closed eyes (about 720/1100 ms), then perform the
same slow upward gaze before Smile. The gaze ends at Smile's neutral starting
pose, avoiding a visible jump. The Smile expression stays while held.
After release, HOLD uses the same destination and temporary timers as TAP:
SLEEP goes to TIRED; TIRED goes to IDLE, subject to the existing pre-07:00 rule.
RELEASE cancels the remaining friendly sequence. Reactions returning to TIRED
ease directly from the displayed pose into half-closed eyes (about 360 ms).
Angry skips its neutral/open-eye ending when returning to TIRED, preventing
a flash of open eyes. This also works when releasing HOLD during its opening
or upward gaze. HOLD detection remains 700 ms. Full state transitions and
scheduled sequences retain their original timing and priority.

## Sound

`src/sound/SoundData.cpp` is the single score catalog for firmware and SDL.
Its constant arrays contain 16-bit frequency/duration pairs (4 bytes per
segment); 0 Hz means a pause. Scores are read directly from flash on ESP32.
SHAKE, TAP, wake-up, sleep and startup previews retain their original notes
and durations. The HOLD reaction now uses the selected CALM variant 3.

Four additional HOLD auditions are available in the simulator: HOLD 1 - WARM
(940 ms), HOLD 2 - CHIRP (670 ms), HOLD 3 - CALM (960 ms), HOLD 4 - PLAYFUL
(945 ms). HOLD - CALM plays the selected reaction sound: variant 3, a low rising
392/494/587 Hz phrase lasting 960 ms, shared by firmware and simulator.
The four comparison buttons remain available. Clicking a sound audition switches to PREVIEW
so automatic animations cannot cut it short; A or AUTO / PREVIEW resumes behavior.

`SoundOutput` separates sound from `AnimationOutput`. `BehaviourProgram`
chooses effects; `SoundManager` handles playback without display dependencies.

| Accepted action | Effect |
|---|---|
| TAP, including another TAP during a reaction | Tap1, restarted once |
| HOLD | Hold once when Smile starts, after the opening/upward gaze; no repeats while held |
| SHAKE_START | Shake1, loop until SHAKE_END or interruption |
| Full TIRED/SLEEP -> IDLE transition | WakeUp, once |
| Full transition into SLEEP | FallAsleep, once |
| Blink, gaze, spontaneous activity, touch return, animation preview | Silent |
| ERROR | Stop sound |

Initial schedule reconciliation in `begin()` is silent. PowerOn and Shake2
remain manual simulator previews. Quick touch opening does not add WakeUp.
An ignored input does not play a sound. Replacing an action cancels its sound,
even when the new action is silent; steps inside one reaction do not retrigger.
Behavior never waits for audio completion and keeps existing state/schedule
priorities and animation timing.

The ESP32 backend targets Arduino Core **3.x** (`ledcAttach`, `ledcWriteTone`,
`ledcWrite`, all pin-based). Verified build environment: Espressif32 8.0.0,
Arduino Core 3.3.12, U8g2 2.36.15. `platform = espressif32` is not version-pinned;
Core 2.x's channel-based API is not supported by this implementation.

LEDC generates the waveform. A single 2 ms periodic `esp_timer` using
`ESP_TIMER_TASK` advances the score independently of the main loop. This matters
because the OLED's full-buffer 100 kHz I2C transfer takes roughly 100 ms.
The callback only services the score/PWM; it neither renders nor logs. There
is no dedicated audio task/stack and no allocation during play/stop. The timer
is created once at initialization; keep the default manager alive for the life
of the firmware, as the global `Robot` does. Future PWM users must account for
LEDC timer sharing when changing frequency.

`play(effect, loop)` always replaces/restarts playback. `stop()` cancels it.
Commands use an atomic latest-request mailbox; PWM changes happen on the next
timer service (nominally within 2 ms, subject to task scheduling). `isPlaying()`
includes pauses and a pending play; a pending stop reports false. Back-to-back
requests before service intentionally keep only the latest one. Delayed service
skips expired segments/cycles instead of extending every note. No `delay()` or
wait for sound completion is used. Initialization/output errors disable audio
without latching the robot's ERROR state; runtime faults are logged once from
the main loop, with best-effort silence and recovery on restart.

On ESP32-C3, ESP-IDF implements the small standard atomic operations with short
critical sections. Only scalar command/status access is protected; LEDC calls
and score processing happen outside those sections.

For host tests or another driver, inject `ToneOutput`, call `begin()` and service
`update(nowMs)` manually. The default ESP32 instance services itself; calls to
its `update()` do nothing. The SDL player uses the same score but generates PCM
on the PC. Its playback-status query is synchronized with the audio callback.
Start synchronization follows the behavior action; physical OLED presentation
still has I2C latency and is not guaranteed to align to a particular tone.

### Piezo hardware remains unverified

GPIO3 controls a **verified driver circuit**, not an assumed 5 V GPIO load.
The exact two-pin "5 V piezoelectric transducer" has not been identified.
Confirm passive/external-drive operation, rated waveform/voltage, current or
capacitance, resonance and the manufacturer's circuit before connecting it.
An active buzzer cannot reproduce these arbitrary tone frequencies.

For 5 V operation, use a suitable 3.3 V-controlled transistor/FET driver with
common ground, a defined OFF state (external pull-down as appropriate), and
the piezo discharge path/protection required by its datasheet. Component values
depend on the actual part. Never route 5 V into GPIO3. Direct 3.3 V GPIO drive
requires a separate check of the transducer and pin-current limits. Firmware
cannot verify the load electrically. No physical upload, electrical measurements
or sound/latency measurements on a connected piezo have been performed.
