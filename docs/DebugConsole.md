# Interaktivní debug konzole DesktopRobot

Vývojový firmware `esp32c3` přijímá řádkové příkazy přes USB Serial na 115200 baud.
V Serial Monitoru nastav zakončení LF nebo CRLF. Po otevření můžeš zadat `help`.

```powershell
pio run -d esp32 -e esp32c3 -t upload
pio device monitor -d esp32 -b 115200
```

## Příkazy

| Příkaz | Význam |
|---|---|
| `help` | Seznam příkazů. |
| `status` | Stav, animace, dokončení, režim chování, hodiny, dočasná reakce, držení, časovač třesení a chyba. |
| `state` | Stejný diagnostický přehled jako `status`. |
| `diag` | Výsledek uložené startup diagnostiky; nic znovu neinicializuje. |
| `wake` | Požadavek na IDLE s ranní sekvencí: otevření podle výchozího stavu, zívnutí a rozhlédnutí. |
| `tired` | Přechod do TIRED přes odpovídající přechodovou animaci. |
| `sleep` | Přechod do SLEEP přes odpovídající sekvenci usínání. |
| `tap` | Stejná událost TAP jako z dotykového vstupu. |
| `hold` | Zahájí přátelskou reakci a držení výrazu. |
| `release` | Ukončí přátelskou sekvenci nebo držení a obnoví správný základní stav. |
| `shake 3000` | SHAKE_START, po 3000 ms SHAKE_END, potom DIZZY. Platný rozsah 1–600000 ms. |
| `time 06:59` | Nastaví simulované místní hodiny na HH:MM:00. |
| `time auto` | Ukončí simulaci a obnoví skutečný dostupný čas a jeho rozvrh. |

Zachovaný příkaz `TIME <local epoch seconds>` nastavuje skutečné záložní
softwarové hodiny. Například `TIME 43200` znamená 12:00:00 dne číslo 0.
Na rozdíl od `time HH:MM` nemění simulované hodiny. Klíčová slova příkazů se
rozpoznávají bez ohledu na velikost písmen; argument `auto` je malými písmeny.

## Události, priority a sekvence

`DebugConsole` pouze rozpoznává příkazy. `Robot` předává injektované i fyzické
události přes stejnou metodu `handleEvent()` do `BehaviourProgram`.
Časované ukončení simulovaného třesení spravuje Robot pomocí uplynulého času;
konzole na nic nečeká. Další `shake` přenastaví dobu držení; během DIZZY opět
spustí SHAKING. Nové vizuální definice se nepřidávají.

Stavové příkazy volají přechody v `BehaviourProgram`, nikoli přehrávání náhledů.
Zachovávají následující plánovanou hranici. Probíhající plánovaná sekvence má
přednost: nižší požadavky mohou být ignorovány podle stejné priority jako senzory.
Při probouzení nebo usínání proto sleduj `status` a před dalším scénářem nech
sekvenci dokončit. Opakovaná žádost o stejný základní stav nedělá nový přechod;
`wake` může v IDLE spustit ranní zívnutí a rozhlédnutí.

V ERROR jsou dostupné pouze `help`, `status`, `state` a `diag`. Běžné příkazy
nemohou ERROR odemknout, restartovat diagnostiku nebo spustit normální animaci.
Dotyk se v ERROR nečte a na dostupném OLED zůstává statická chyba.

## Skutečný a simulovaný čas

Simulované hodiny běží normální rychlostí, včetně přechodu přes půlnoc.
Příkaz HH:MM zachovává číslo aktuálního simulovaného dne; při prvním zapnutí
simulace převezme dostupný skutečný den, jinak použije den 0. Menší HH:MM znamená
záměrný skok zpět v témže dni. Neodvozuje automaticky následující den.

Rozvrh používá 07:00 → IDLE, 21:00 → TIRED a 21:30 → SLEEP. Při skoku přes více
hranic se zpracuje výsledný platný stav; nepřehrává se zpětně celý vynechaný den.
Hranice se pamatuje podle dne a časového úseku. Opakované nastavení stejné hranice
ani následný průchod loop tedy nerestartují probíhající plánovanou sekvenci.

Skutečné hodiny pokračují nezávisle i během simulace. `time auto` odstraní dočasné
debug návraty a časovač simulovaného třesení a srovná chování s aktuálním reálným
rozvrhem. Probíhající plánovaný přechod ke stejnému cíli se znovu nespouští.

AUTO nyní poskytuje `TimeManager`: po diagnostice se pokusí připojit ESP32
k Wi-Fi a ověřit čerstvou NTP odpověď. Po pokusu rádio vypne. Konfigurace,
timeouty a testování jsou v [návodu Wi-Fi a času](WifiTime.md).
Ruční číselný TIME má přednost: automatické pokusy zruší do restartu a ani
`time auto` tento zámek neruší. Status ukazuje také zdroj času, stav rádia a
poslední chybu synchronizace. Bez NTP nebo ručního zdroje zůstává AUTO UNSET.
Ruční i simulované hodiny se po restartu neobnovují z trvalé paměti.

## Ověření scénářů

1. `sleep`, počkat na dokončení, `wake`: otevření, YAWN, LOOK AROUND a IDLE.
2. `tap`: ANGRY a návrat podle aktuálního stavu; ve spánku nejdřív probuzení.
3. `hold`, počkat na SMILE, `release`: konec držení a správný návrat.
4. `shake 3000`: SHAKING tři sekundy, potom DIZZY. Nové zatřesení přeruší DIZZY.
5. `time 06:59`, počkat minutu: ranní přechod v 07:00.
6. `time 20:59`, počkat minutu: přechod do TIRED v 21:00.
7. `time 21:29`, počkat minutu: přechod do SLEEP v 21:30.
8. Opakovat `time 21:30`: nesmí restartovat již běžící usínání.
9. `time auto`, `status`: návrat k nezávislým skutečným hodinám nebo výslovné UNSET.
10. Po chybovém bootu ověřit `diag`, `status` a odmítnutí `wake` či `tap`.

Log obsahuje `[STATE]`, `[ANIM] started/finished`, `[EVENT]`, `[CLOCK]`,
`[REQUEST]` a `[ERROR]`. Hláška o přijetí události neznamená, že přebila vyšší
prioritu; uvádí i aktuální režim chování.

## Paměť, produkční sestavení a testy

Vstupní řádek má maximálně 95 znaků. Příliš dlouhý řádek se celý zahodí.
Za průchod se načte nejvýše 64 znaků a vykoná nejvýše jeden příkaz.
Výstup má pevnou frontu 2048 bajtů; zapisuje se pouze do volné kapacity Serial,
nejvýše 64 bajtů za průchod. Při zahlcení se zprávy zahazují s následným oznámením.
Animace ani časovače nečekají na čtení konzole počítačem. Nepoužívá se Arduino
String ani dynamická fronta příkazů.

```powershell
./esp32/host/run.ps1
pio run -d esp32 -e esp32c3
pio run -d esp32 -e esp32c3-production
```

Produkční prostředí definuje `ROBOT_DEBUG_CONSOLE=0`: parser, vstupní buffer a
výstupní fronta se nekompilují. Startup diagnostika a původní provoz zůstávají.
Testy konzole používají řízený Serial a čas; nenahrazují fyzický test USB a OLED.
