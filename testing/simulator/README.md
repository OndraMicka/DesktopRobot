# Simulator animation previews

Build with `./testing/simulator/build.ps1` from the repository root. Close the
running simulator before rebuilding, then launch `testing/build/desktop_robot_simulator.exe`.

After the startup animation, the state columns contain these manual previews:

| State | Buttons |
| --- | --- |
| IDLE | NEW - HMM, NEW - KISS, NEW - STARTLE LEFT, NEW - STARTLE UP RIGHT |
| TIRED | NEW - MICROSLEEP, NEW - SIGH, NEW - HEAVY LIDS, NEW - SLEEPY NOD |
| SLEEP | NEW - SLEEP ROLL, NEW - SLEEP TWITCH |

Previews use the existing solid eyes and thick mouth strokes, with filled
geometry for the heart. HMM uses a subtle asymmetric expression and a slowly
shifted mouth. KISS closes its eyes, briefly puckers, sends a heart, then relaxes
into a smile before opening its eyes. The filled heart keeps its two lobes,
top cleft and pointed bottom throughout flight. STARTLE LEFT glances left in
100 ms with almost normal-sized eyes, watches for about two seconds, then
relaxes and looks back more slowly. STARTLE UP RIGHT uses the same timing for
a glance diagonally up and right. Both keep the neutral mouth throughout.
Microsleep retains its approved timing.

HEAVY LIDS lets the left upper lid fall first, then the right. Both eyes open
with effort to a modest height, hold briefly, then gradually settle to TIRED.
SLEEPY NOD closes the eyes during one small, slow downward nod and reopens them
partway during the return. SLEEP TWITCH stays still for a second, gives one
brief two-pixel twitch and softly settles without opening its eyes. These
three previews keep a neutral mouth and the original solid rounded geometry.

Click a button to play the complete preview once and return to that state's
base loop. New previews switch AUTO off so scheduled behavior cannot interrupt
them. Click again to replay; press A to resume automatic behavior.

All ten animations are also available to firmware and simulator AUTO behavior.
Their reviewed authoring reference remains in `src/animation/PreviewAnimations.*`;
`esp32/src/display/animations/CharacterAnimationData.inc` stores their identical
frames in flash. Weights and cooldowns are configured in `BehaviourProgram.h`
and documented in `esp32/README.md`. ERROR is unchanged.

To regenerate flash data after an approved visual edit, build the CMake target
`export_character_animations`, then pass the destination
`esp32/src/display/animations/CharacterAnimationData.inc` to the exporter from
the repository root. Run `./esp32/host/run.ps1` to verify the new data against
every pixel and frame duration of the authoring reference.

The CMake/CTest target `preview_animations` checks all ten previews for exact
base-state endpoints, varied frames, durations, solid geometry, drawing-command
budgets, both glance directions and their unchanged neutral mouth. It also
checks the asymmetric lids, single nod, closed sleeping eyes and limited twitch,
and compares each firmware frame against the reviewed reference.
