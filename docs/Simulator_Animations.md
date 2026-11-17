# Animace simulátoru

Tato stránka popisuje animace dostupné v simulátoru a firmware. Kód simulátoru
je v `testing/simulator/src/animation/`; katalog firmware je v
`esp32/src/display/animations/`. Deset schválených charakterových animací
sdílí automatické chování a firmware data v `CharacterAnimationData.inc`.

## Základní a přechodové animace

| Skupina | Identifikátory |
| --- | --- |
| IDLE | Blink, LookLeft, LookRight, LookUp, LookAround, Thinking, IdleYawn, Shaking, Dizzy, Angry, Smile, Bored, Listening, RandomCuriosity |
| TIRED | TiredYawn, SlowBlink, SlowEyeMovement |
| SLEEP | SleepEyeOpen, SleepZ |
| Přechody | WakeUp, TiredToSleep, IdleToTired, IdleToSleep, TiredToIdle, SleepToTired |
| Základní smyčky | IDLE, TIRED, SLEEP |

`TiredToSleep` je aktuální název přechodu TIRED → SLEEP. `Dizzy` se při
zotavení po zatřesení sestavuje z právě zobrazené pózy, takže naváže na
poslední snímek SHAKING.

## Charakterové aktivity

| Stav | Animace |
| --- | --- |
| IDLE | Hmm, Kiss, StartleLeft, StartleUpRight |
| TIRED | Microsleep, Sigh, HeavyLids, SleepyNod |
| SLEEP | SleepRoll, SleepTwitch |

Celkem je 35 katalogových jednorázových animací a tři základní stavové smyčky.
Aktuální váhy, cooldowny, ovládání náhledů a export firmware dat jsou popsány v
[návodu simulátoru](../testing/simulator/README.md) a
[průvodci firmwarem](../esp32/README.md).
