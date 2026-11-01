# DesktopRobot — Behavior and Animation Specification

## 1. Project Overview

DesktopRobot is a small animated desktop companion with a 128×64 monochrome OLED display, a capacitive touch sensor, an MPU6050 motion sensor, a passive piezo buzzer, and an ESP32-C3 microcontroller.

The robot should feel like a small character with its own daily routine, emotions, reactions, and spontaneous behaviors.

Its face must be drawn programmatically, pixel by pixel, rather than using GIFs or pre-rendered images.

The project has two rendering targets:

* **PC simulator:** C++ and SDL2.
* **Physical device:** ESP32-C3 and SH1106 OLED.

Both targets must display the same logical 128×64 framebuffer.

The architecture should follow:

`Input → Behavior System → Animation System → Framebuffer → Renderer`

The behavior system decides what the robot does. The animation system controls how it looks while doing it. The renderer only displays the framebuffer.

---

## 2. Main States

The robot has three permanent behavioral states.

### IDLE

Normal daytime behavior.

* Eyes are open.
* The robot blinks at irregular intervals.
* Eyes occasionally look in different directions.
* The robot performs occasional spontaneous activities.
* It may hum a short melody using the passive buzzer.
* It reacts to touch and movement.
* Movements should feel natural rather than perfectly repetitive.

### TIRED

A sleepy state that occurs before bedtime or after waking up earlier than scheduled.

* Eye movements become slower.
* The robot occasionally yawns.
* Eyes sometimes become half-closed.
* Blinking becomes slower.
* The robot looks sleepy and less energetic.
* Spontaneous activities become less frequent.
* It can occasionally start closing its eyes, then open them again.

### SLEEP

The robot is sleeping.

* Eyes are normally closed.
* Small `Z` characters occasionally appear above the eyes.
* The `Z` characters move upward and gradually disappear.
* The robot occasionally opens its eyes slightly, then closes them again.
* Other animations are greatly reduced.
* The robot should not perform normal daytime activities.

---

## 3. Daily Schedule

Use local time. The schedule must be independent of animation frame rate.

| Time  | Event              | Resulting state |
| ----- | ------------------ | --------------- |
| 07:00 | Wake-up time       | IDLE            |
| 21:00 | Evening transition | TIRED           |
| 21:30 | Bedtime            | SLEEP           |

These are fixed schedule boundaries, not delays measured from the last interaction.

### 3.1 Evening Transition: IDLE → TIRED

At 21:00:

1. Finish or safely interrupt the current spontaneous activity.
2. Perform a yawn animation.
3. Gradually make the eyes look sleepier.
4. Enter TIRED.

### 3.2 Bedtime: TIRED → SLEEP

At 21:30:

1. Stop daytime spontaneous activities.
2. Slowly close the eyes.
3. Hold the closed-eye expression briefly.
4. Enter SLEEP and begin the sleep behavior.

### 3.3 Morning Wake-Up: SLEEP → IDLE

At 07:00:

1. Slowly open the eyes.
2. Perform a yawn.
3. Optionally look around as if waking up.
4. Return to the normal daytime behavior.
5. Enter IDLE.

The wake-up sequence should feel gradual, not instantaneous.

---

## 4. Early Wake-Up and Alarm Behavior

The robot must distinguish between being physically woken up and reaching its scheduled wake-up time.

**Touching the robot must not reset or cancel its daily schedule.**

Example: the robot is sleeping and is touched at 06:00.

* It wakes up and reacts to the touch.
* After the reaction, it enters TIRED.
* It remains TIRED until 07:00.
* At 07:00, it performs the normal morning wake-up sequence and enters IDLE.

Another example: the robot is touched at 06:55.

* It performs the touch reaction.
* It remains TIRED until 07:00, unless the reaction itself is still playing.
* At 07:00, it transitions to IDLE.

The scheduled transition takes priority over a temporary interaction state, but must not abruptly cut off an important animation unless necessary.

The same principle applies if an interaction crosses 21:00 or 21:30: the current schedule determines the appropriate permanent state.

---

## 5. Automatic Behavior in IDLE

The robot should not simply repeat a fixed animation loop. It should combine regular small movements with occasional spontaneous activities.

### 5.1 Regular Eye Behavior

* Random blinking.
* Looking left.
* Looking right.
* Looking upward.
* Looking around in several directions.
* Returning the eyes to the center.
* Briefly holding a gaze in one direction.

These actions must have variable intervals and durations.

### 5.2 Spontaneous Activities

The robot may occasionally perform one of the following activities.

#### A. Looking Around

1. Look left.
2. Look right.
3. Return to the center.

#### B. Thinking

1. Look upward.
2. Hold the gaze.
3. Optionally blink.
4. Return to normal.

#### C. Casual Yawn

1. Slowly narrow the eyes.
2. Open the mouth.
3. Hold the yawn briefly.
4. Close the mouth and reopen the eyes.

This should be less frequent than yawning in TIRED.

#### D. Humming

1. Optionally show a small smile.
2. Play a short sequence of notes on the passive buzzer.
3. Optionally animate the mouth or eyes in rhythm.
4. Return to normal.

#### F. Contented Expression

1. Close the eyes gently.
2. Show a subtle happy expression.
3. Optionally play a pleasant short tone.
4. Return to normal.

#### G. Checking the Surroundings

1. Quickly look in several directions.
2. Pause briefly.
3. Return to the center.

#### H. Brief Freeze

1. Stop eye movement.
2. Hold a neutral expression for approximately 1–2 seconds.
3. Resume normal behavior.

#### I. Bored Expression

1. Look sideways.
2. Show an annoyed or bored expression.
3. Optionally play a short low-pitched sound.
4. Return to normal.

#### J. Self-Satisfied Smile

1. Briefly show a subtle smile.
2. Hold it for a moment.
3. Return to the normal face.

#### K. Almost Falling Asleep

1. Slowly close the eyes.
2. Keep them closed briefly.
3. Suddenly open them again.
4. Return to normal as if nothing happened.

This activity should be rare and should not change the permanent state to TIRED or SLEEP.

#### L. Listening

1. Stop moving the eyes.
2. Hold a focused expression.
3. Optionally look in one direction.
4. Resume normal behavior.

#### M. Random Curiosity

1. Look quickly to one side.
2. Look in the opposite direction.
3. Return to the center.

This should look like the robot noticed something interesting.

---

## 6. Spontaneous Activity Scheduling

Do not select a random animation every few seconds.

Instead, implement an activity scheduler with:

* Random intervals between activities.
* Individual activity probabilities or weights.
* Per-activity cooldowns.
* A check that the current state permits the activity.
* A mechanism to prevent two activities from playing simultaneously.
* The ability to interrupt or cancel an activity when an important event occurs.

The current decision intervals are 2–6 seconds in IDLE, 4–11 seconds in TIRED,
and 7–16 seconds in SLEEP. The quiet outcome has weight 3. RandomCuriosity has
weight 15, Bored 8 and SleepZ 10. Blink delays are 2–4 seconds in IDLE and
4–8 seconds in TIRED. Activity completions and quiet draws preserve the blink
deadline. A random 0.8?2.2 second pause follows each spontaneous gesture. If
both deadlines are overdue, the older one runs first; exact ties are randomized.
Blinking does not force an activity to follow immediately. Playback timing and the no-immediate-repeat rule remain unchanged.
Current cooldowns are defined in
`esp32/src/behavior/BehaviourProgram.h`.

The reviewed Microsleep, Hmm, Kiss, Sigh, SleepRoll, StartleLeft,
StartleUpRight, HeavyLids, SleepyNod and SleepTwitch animations are included in
automatic selection in their respective states. Current weights and shortened
cooldowns are listed in `esp32/README.md`. Both startle directions additionally
share a 20-second cooldown, measured between animation starts. Immutable flash
frames match the reviewed simulator geometry and timing exactly.

These values should be configurable and adjustable after testing.

Example initial weights for IDLE:

| Activity              | Initial weight |
| --------------------- | -------------: |
| Looking around        |             35 |
| Thinking              |             10 |
| Casual yawn           |              8 |
| Humming               |              7 |
| Contented expression  |              5 |
| Checking surroundings |              5 |
| Bored expression      |              5 |
| Self-satisfied smile  |              5 |
| Brief freeze          |              5 |
| Almost falling asleep |              3 |
| Listening             |              7 |

These are relative weights, not percentages. Normalize the eligible weights when selecting an activity.

Ordinary blinking should be scheduled separately and must not count as a spontaneous activity.

Not every decision must produce an activity. The robot may simply continue its normal behavior.

Rare activities should have longer cooldowns to avoid repetitive behavior.

---

## 7. Automatic Behavior in TIRED

The robot should appear visibly sleepier than in IDLE.

Possible activities:

* Occasional yawning.
* Slow blinking.
* Half-closed eyes.
* Slow eye movements.
* Briefly closing the eyes.
* Looking sleepy and struggling to stay awake.

The activities must not cause a permanent state change by themselves unless the scheduled bedtime or another explicit rule requires it.

At 21:30, the robot enters SLEEP through the bedtime transition.

If the robot is in TIRED because it was woken before 07:00, it must remain TIRED until 07:00.

---

## 8. Automatic Behavior in SLEEP

Sleep behavior should be calm and minimal.

### 8.1 Closed Eyes

The default sleeping expression uses closed eyes.

### 8.2 Z Animation

Periodically:

1. Show one or more `Z` characters.
2. Move them upward.
3. Gradually remove or fade them.
4. Wait before repeating.

Because the display is monochrome, fading may be simulated by blinking, shortening the visible duration, or removing pixels in stages.

### 8.3 Slight Eye Opening

Occasionally:

1. Slightly open the eyes.
2. Hold the expression briefly.
3. Close the eyes again.

The robot must remain in SLEEP throughout this animation.

### 8.4 Restrictions

While sleeping:

* Do not run normal daytime spontaneous activities.
* Do not hum randomly.
* Do not perform ordinary blinking.
* Allow only sleep animations and reactions to relevant events.

---

## 9. Touch Input

The TTP223 touch sensor must support two distinct interactions:

* A short tap.
* A long press.

The behavior system must distinguish the duration of the touch.

Suggested initial thresholds:

| Touch duration   | Interpretation         |
| ---------------- | ---------------------- |
| Less than 300 ms | TAP                    |
| 300–700 ms       | Pending classification |
| More than 700 ms | HOLD                   |

These values should be configurable.

Do not trigger both TAP and HOLD for the same interaction.

The system must also detect the release of a held touch.

Suggested events:

```cpp
enum class InputEvent {
    NONE,
    TAP,
    TOUCH_HOLD,
    TOUCH_RELEASE,
    MOTION
};
```

The simulator should be able to generate these events without physical hardware.

---

## 10. Reaction to a Short Tap

A short tap should make the robot feel startled and annoyed.

### 10.1 Tap During IDLE

Sequence:

1. Show an angry expression.
2. Play a short, annoyed buzzer sound.
3. Hold the expression briefly.
4. Return to IDLE.

The reaction should be playful rather than aggressive.

### 10.2 Tap During TIRED

Sequence:

1. Briefly open the eyes before the irritated expression.
2. Transition to an angry or irritated expression.
3. Play a short, annoyed buzzer sound.
4. Enter IDLE temporarily.
5. After 5 minutes, return to TIRED.
6. After another 5 minutes, enter SLEEP.

The scheduled time boundaries still apply. If 07:00, 21:00, or 21:30 occurs during this sequence, the schedule must determine the appropriate state.

### 10.3 Tap During SLEEP

Sequence:

1. Immediately interrupt the sleep animation.
2. Show a startled expression.
3. Transition to an angry or irritated expression.
4. Play a short, annoyed buzzer sound.
5. Enter TIRED temporarily.
6. After 5 minutes, return to SLEEP, unless a scheduled transition determines otherwise.

If the robot is woken before 07:00, it must remain TIRED until 07:00 instead of returning to SLEEP after five minutes.

The reaction should communicate: "Why are you waking me up?"

---

## 11. Reaction to a Long Press

A long press should produce a friendly interaction instead of an angry one.

The robot should look upward toward the touch sensor and then smile.

### 11.1 Long Press During IDLE

Sequence:

1. Interrupt the current spontaneous activity if necessary.
2. Move the eyes upward.
3. Hold the upward gaze.
4. Show a smile.
5. Play a pleasant short tone or melody.
6. Maintain the expression while the touch is held.
7. When the touch is released, return to IDLE.

### 11.2 Long Press During TIRED

Sequence:

1. Use a dedicated TIRED hold animation to slowly open the half-closed eyes, then look upward with the original slow gaze timing.
2. Continue seamlessly into Smile.
3. Play a pleasant sound.
4. Maintain the expression while the touch is held.
5. When the touch is released, enter IDLE with the same temporary wake-up timer as TAP (except the existing early-morning rule).

The interaction must not reset the sleep schedule.

### 11.3 Long Press During SLEEP

Sequence:

1. Interrupt the sleeping animation.
2. Use a dedicated SLEEP hold animation to slowly open the closed eyes (slower than TIRED); do not use the quick TAP opening.
3. Look upward toward the touch sensor.
4. Show a friendly smile.
5. Play a pleasant short tone or melody.
6. Maintain the expression while the touch is held.
7. When the touch is released, enter TIRED.

If the touch occurs before 07:00, remain TIRED until 07:00.

If the touch occurs during the night after bedtime, use the same temporary wake-up rule as other night interactions: remain TIRED for approximately five minutes, then return to SLEEP, unless a scheduled transition takes priority.

If the robot is touched during a sleep-to-wake transition, do not restart the wake-up animation from the beginning unnecessarily.

---

## 12. Buzzer and Sound Design

Use the passive piezo buzzer to complement the facial animations.

The sound must be generated programmatically using frequencies and durations, not audio files.

### Current implemented sound routing

The shared `esp32/src/sound/SoundData.cpp` catalog defines all seven tone/pause
scores for both firmware and SDL. `BehaviourProgram` controls a separate
`SoundOutput`; the display output and animation data have no sound dependency.

* Accepted TAP: Tap1 once; every accepted new tap restarts it.
* Accepted HOLD: opening and LookUp remain silent. Play Hold once when the
  reaction enters Smile; keeping the smile held never retriggers it. RELEASE
  stops the reaction sound; release before Smile produces no Hold sound.
* SHAKE_START: Shake1 loops; SHAKE_END stops it before recovery. Duplicate
  SHAKE_START and rejected touch events do not restart audio.
* Full TIRED/SLEEP -> IDLE transitions: WakeUp once. Quick touch opening uses
  only its TAP/HOLD sound, with no additional wake-up effect.
* Full transitions to SLEEP: FallAsleep once.
* Blink, gaze, spontaneous activities, touch returns and animation previews
  are silent. Replacing an action with a silent one stops the previous sound.
* ERROR stops audio. Schedule priority applies to sound as well as animation.
* Initial schedule reconciliation in `begin()` is silent. PowerOn and Shake2
  are standalone simulator previews; spontaneous humming is not implemented.

Only one score plays at a time. New requests replace old playback; behavior
never waits for sound completion and retains its existing state/timer rules.
ESP32 LEDC generates tones on GPIO3; a 2 ms task-dispatched ESP Timer services
segments independently of blocking OLED transfers. Starts follow logical action
boundaries, with no guarantee of exact physical frame/tone alignment. Playback
status includes pauses and pending play requests; stop is applied on the next
timer service. SDL uses the same notes/durations with its existing PCM backend.
The sections below retain broader sound-design goals beyond this initial routing.

### 12.1 Angry Sound

Triggered by a short tap.

* Short and sharp.
* One to three notes or a short frequency sweep.
* Slightly dissonant or descending.
* Clearly different from the friendly sound.

### 12.2 Friendly Sound

Triggered by a long press.

* Pleasant and soft.
* One or more simple notes.
* A short, friendly melody is acceptable.
* Avoid excessive repetition while the sensor remains held.

### 12.3 Spontaneous Humming

During IDLE:

* Occasionally play a short melody.
* Use a small collection of predefined note sequences.
* Optionally synchronize eye or mouth animation with the notes.
* Apply a cooldown so the sound does not become annoying.

### 12.4 Optional Additional Sounds

* A short sound during the morning wake-up.
* A quiet tone during the contented expression.
* A low, brief sound during the bored expression.

### 12.5 Sound Restrictions

* Do not play spontaneous sounds during SLEEP.
* Do not overlap buzzer sequences.
* Important interaction sounds may interrupt an ordinary spontaneous melody.
* All sound durations and frequencies must be configurable.
* Provide a way to disable spontaneous sounds independently of interaction sounds.

---

## 13. State and Interaction Priority

The system must separate permanent states, temporary animations, and input events.

Permanent states:

* IDLE
* TIRED
* SLEEP

Temporary animations and reactions:

* BLINK
* LOOK_LEFT
* LOOK_RIGHT
* LOOK_UP
* LOOK_AROUND
* YAWN
* ANGRY
* SMILE
* WAKE_UP
* SLEEP_EYE_OPEN
* HUM
* SLEEP_Z

Events:

* TAP
* TOUCH_HOLD
* TOUCH_RELEASE
* MOTION
* SCHEDULE_TRANSITION

The behavior system should remember the appropriate state to return to after a temporary animation.

Examples:

* `IDLE → ANGRY → IDLE`
* `TIRED → YAWN → TIRED`
* `SLEEP → SLEEP_EYE_OPEN → SLEEP`
* `SLEEP → WAKE_UP → TIRED`
* `IDLE → LOOK_UP → SMILE → IDLE`

Do not implement every temporary reaction as a new permanent state.

### Priority Order

1. Scheduled transitions.
2. User interactions.
3. Required state-transition animations.
4. Spontaneous activities.
5. Ordinary eye movements and blinking.

Higher-priority events should be able to interrupt lower-priority behavior safely.

Do not allow multiple animations to overwrite the framebuffer concurrently.

If an interaction arrives while another temporary animation is playing, interrupt or replace the existing animation according to the priority rules.

---

## 14. Animation Requirements

All animations must be generated programmatically.

The display resolution is exactly 128×64 pixels.

Coordinates:

* X: 0–127.
* Y: 0–63.

Each rendered frame must produce the exact final pixel content.

The PC simulator must enlarge each logical OLED pixel using SDL2. The ESP32 renderer must display the same framebuffer on the SH1106 OLED.

Keep animation logic independent of SDL2 and hardware-specific OLED calls.

Suggested organization:

```text
DesktopRobot/
├── simulator/
│   └── src/
│       ├── main.cpp
│       ├── behavior/
│       ├── animations/
│       ├── framebuffer/
│       └── renderers/
├── firmware/
│   └── src/
│       ├── behavior/
│       ├── animations/
│       ├── framebuffer/
│       └── renderers/
└── README.md
```

Adapt the actual file structure to the existing repository instead of duplicating code unnecessarily.

The behavior and animation logic should be reusable between the simulator and the physical device.

Use elapsed time rather than frame counts for animation timing, activity intervals, touch thresholds, and state transitions.

---

## 15. Simulator GUI

The SDL2 simulator should allow manual testing of behavior.

Provide buttons grouped by function.

### Permanent States

* IDLE
* TIRED
* SLEEP

### One-Shot Animations

* BLINK
* YAWN
* ANGRY
* WAKE_UP
* LOOK_UP
* SMILE

### Input Simulation

* TAP
* HOLD
* RELEASE
* MOTION

The GUI should also allow testing the scheduled transitions.

Useful optional controls:

* Current simulated time.
* A way to change the simulated time.
* A way to advance time quickly.
* A way to enable or disable spontaneous activities.
* A way to enable or disable sounds.
* A way to trigger spontaneous activities manually.

The simulator should show the current permanent state and active temporary animation.

Buttons must be clickable, show a hover effect, and clearly indicate the active permanent state.

Manual controls should not silently corrupt the schedule or leave the behavior system in an impossible state.

---

## 16. Implementation Requirements

* Preserve the existing project and build configuration wherever possible.
* Do not introduce unnecessary external dependencies.
* Do not use GIF, PNG, or JPG animations.
* Do not put SDL2 calls inside the behavior or animation logic.
* Do not use blocking delays such as `delay(5000)` for behavior scheduling.
* Use non-blocking timers based on elapsed time.
* Avoid overlapping animations and buzzer sequences.
* Make random behavior configurable and reproducible during simulator testing when needed.
* Keep the daily schedule separate from temporary interactions.
* Ensure the code can run on the ESP32-C3 with limited memory.
* Keep the 128×64 framebuffer as the single source of truth for displayed pixels.
* Do not implement hardware-specific sensor behavior in the shared animation logic.

---

## 17. Acceptance Tests

The implementation should pass the following scenarios.

### Daily Schedule

* At 07:00, SLEEP transitions through the wake-up sequence to IDLE.
* At 21:00, IDLE transitions through a yawn to TIRED.
* At 21:30, TIRED closes its eyes and transitions to SLEEP.
* SLEEP animations continue without running daytime activities.

### Early Wake-Up

* Waking the robot at 06:00 results in TIRED.
* It remains TIRED until 07:00.
* At 07:00, it transitions to IDLE.
* A touch does not reset the scheduled wake-up time.

### Tap

* TAP during IDLE causes an angry reaction and a short annoyed sound, then returns to IDLE.
* TAP during TIRED causes an irritated expression, then temporarily enters IDLE before returning to TIRED and eventually SLEEP according to the specified timing.
* TAP during SLEEP causes an irritated expression, then enters TIRED.
* A night-time wake-up before 07:00 remains TIRED until 07:00.

### Long Press

* HOLD during IDLE makes the robot look up, smile, and play a pleasant sound.
* HOLD during TIRED produces the same friendly interaction but returns to TIRED.
* HOLD during SLEEP wakes the robot, makes it look up and smile, and leaves it TIRED after release.
* Releasing the touch restores the correct state and schedule.

### Spontaneous Activities

* IDLE occasionally performs different activities.
* Activities do not all play at fixed intervals.
* Individual cooldowns prevent immediate repetition.
* Spontaneous activities do not overlap.
* SLEEP does not run ordinary daytime activities.
* TIRED uses slower and sleepier behavior than IDLE.

### Rendering and Timing

* All frames have a logical resolution of 128×64.
* The SDL2 simulator renders the framebuffer correctly.
* Animation timing does not depend on FPS.
* The code remains compatible with the intended ESP32-C3 and SH1106 hardware architecture.

---

## 18. Recommended Development Order

Implement the system in stages.

1. Finalize the framebuffer and renderer architecture.
2. Implement IDLE, TIRED, and SLEEP animations.
3. Implement the daily schedule and wake-up transitions.
4. Implement TAP and HOLD event handling.
5. Implement angry and friendly reactions.
6. Implement buzzer tone generation and sequencing.
7. Implement the spontaneous activity scheduler.
8. Add simulator controls for states, events, and simulated time.
9. Test interruption, timing, and state restoration.
10. Prepare the shared logic for integration with the physical ESP32-C3 hardware.

**Final goal:** DesktopRobot should feel like a small, expressive companion with a daily routine, distinct moods, spontaneous behaviors, and meaningful reactions to touch, while remaining predictable enough to test and reliable enough to run on a microcontroller.


## 17. Shake Reaction

The MPU6050 input layer must expose `SHAKE_START` and `SHAKE_END` events.
Use configurable detection thresholds and a quiet-time debounce before emitting
`SHAKE_END`, so individual acceleration peaks do not restart the reaction.
The sensor driver and threshold tuning remain firmware implementation work.

* `SHAKE_START`: interrupt the current activity and loop `SHAKING` for as long
  as shaking continues. Use narrowed, inward-sloping eyes and short restrained
  side-to-side movements. Shape the eyes themselves; do not draw eyebrows.
* `SHAKE_END`: stop the loop immediately and play `DIZZY` once, blending from
  the exact displayed shake pose. The face circles and rolls with a lagging gaze,
  then squeezes its eyes and shakes side to side while still dizzy. The circling
  fades during this shake, which clears its head, before the eyes open into a
  calm neutral expression.
* A new `SHAKE_START` during `DIZZY` interrupts recovery and resumes `SHAKING`.
* After recovery, return to the appropriate permanent state. Shaking does not
  change the daily schedule.
* Keep the same solid eyes and simple mouth as the existing expressions;
  no hearts, stars, blush or decorative effects.

Simulator: hold Space or the `SHAKE - HOLD` button for the first phase; release
it to trigger recovery. `DIZZY` can also be previewed separately. The selected
IDLE/TIRED/SLEEP state is preserved across both phases.

`SURPRISED` has been removed from the animation catalogue. `ANGRY` is a restrained
frown formed by the upper edges of the eyes, without eyebrows. `SMILE` holds
its expression longer before returning to neutral.


## 18. State Transition Previews

All six directed transitions have dedicated, non-looping animations:

| From | To | Animation |
| --- | --- | --- |
| IDLE | TIRED | IDLE TO TIRED: narrow eyes, yawn, settle into tired eyes |
| IDLE | SLEEP | IDLE TO SLEEP: yawn, droop, close eyes and pause |
| TIRED | IDLE | TIRED TO IDLE: blink, open eyes and look around |
| TIRED | SLEEP | GO TO SLEEP: droop, briefly resist, close eyes |
| SLEEP | IDLE | WAKE UP: existing gradual wake-up sequence |
| SLEEP | TIRED | SLEEP TO TIRED: tentative opening, blink, sleepy gaze |

In the simulator, SET STATE chooses the transition from the selected source
state, plays it once and continues the target state's loop. Choosing the same
state does nothing. Each transition also has its own preview button in its
source state's panel; previews finish in the target state. The STATE indicator
shows the destination as soon as a transition starts. A new selection can
interrupt the transition and uses that destination as its source state.
