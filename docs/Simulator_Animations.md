# Animace simulátoru

Definice simulátoru jsou v `testing/simulator/src/animation/TestAnimations.cpp`.
Firmware má samostatné třídy jednotlivých animací v `esp32/src/display/animations/`.

## Animace pro stav IDLE

| Identifikátor | Název v simulátoru |
|---|---|
| `Blink` | BLINK |
| `LookLeft` | LOOK LEFT |
| `LookRight` | LOOK RIGHT |
| `LookUp` | LOOK UP |
| `LookAround` | LOOK AROUND |
| `Thinking` | THINKING |
| `IdleYawn` | YAWN |
| `Shaking` | SHAKE - HOLD (animace SHAKING) |
| `Dizzy` | DIZZY |
| `Angry` | ANGRY |
| `Smile` | SMILE |
| `Bored` | BORED |
| `Listening` | LISTENING |
| `RandomCuriosity` | RANDOM CURIOSITY |

## Animace pro stav TIRED

| Identifikátor | Název v simulátoru |
|---|---|
| `TiredYawn` | YAWN |
| `SlowBlink` | SLOW BLINK |
| `SlowEyeMovement` | SLOW EYE MOVEMENT |

## Animace pro stav SLEEP

| Identifikátor | Název v simulátoru |
|---|---|
| `SleepEyeOpen` | SLEEP EYE OPEN |
| `SleepZ` | ZZZ |

## Přechodové animace

| Identifikátor | Název v simulátoru | Přechod |
|---|---|---|
| `WakeUp` | WAKE UP - TO IDLE | SLEEP → IDLE |
| `TiredToSleep` | TIRED TO SLEEP | TIRED → SLEEP |
| `IdleToTired` | IDLE TO TIRED | IDLE → TIRED |
| `IdleToSleep` | IDLE TO SLEEP | IDLE → SLEEP |
| `TiredToIdle` | TIRED TO IDLE | TIRED → IDLE |
| `SleepToTired` | SLEEP TO TIRED | SLEEP → TIRED |

## Základní stavové animace

| Název | Základní výraz |
|---|---|
| IDLE | Neutrální obličej |
| TIRED | Unavený obličej |
| SLEEP | Zavřené oči |

Celkem 25 položek v katalogu a 3 základní stavové animace.
Po uvolnění třesení se navíc generuje varianta DIZZY navazující na aktuální pózu.
