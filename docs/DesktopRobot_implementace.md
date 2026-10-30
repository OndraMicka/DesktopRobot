# DesktopRobot – zadání implementace firmware

## Cíl
Implementuj modulární firmware pro **ESP32-C3 Super Mini**, OLED **SH1106 128×64 přes I²C** a kapacitní dotykový senzor **TTP223**. Jde o implementaci architektury, kterou jsme navrhli; **nepřidávej zatím komplexní denní rozvrh ani zvuky**. Kód musí jít sestavit a nahrát pomocí PlatformIO. Zachovej existující projekt, funkční ovladače, kreslicí primitiva a animace, pokud už jsou v repozitáři; před změnami je prozkoumej a zbytečně je nepřepisuj.

## Základní principy
- `main.cpp` pouze inicializuje systém a opakovaně volá aktualizace; **neobsahuje chování ani kreslení**.
- `BehaviourProgram` je **jediné místo pro rozhodování o chování**: stavový automat, reakce na události, volba animací, priority a přechody.
- `InputController` zpracovává dotyk a generuje události `TAP`, `HOLD` (a případně `RELEASE`); **nevybírá animace**.
- `OutputDisplay` dostane například `playAnimation(AnimationId::YAWN)`, vykresluje a časuje animaci; **nerozhoduje, kdy se má YAWN spustit**.
- Hardcoded definice animací jsou ve složce `animations/`, odděleně od `BehaviourProgram` i `OutputDisplay`.
- `AnimationPlayer` spravuje přehrávání snímků a stav dokončení; nepoužívej blokující `delay()`.
- `Pins.h` obsahuje **předem definované GPIO**; testování nemá za úkol piny automaticky hledat nebo přepisovat.
- `HardwareDiagnostics` se spustí **při každém zapnutí**, před běžným chováním. Kritická chyba přepne systém do `ERROR` režimu.
- V `ERROR` režimu robot **nedělá nic jiného**, než že na fungujícím OLED drží **smutný obličej a text `ERROR`**; současně vypíše příčinu na Serial, je-li dostupný. Nezpracovává dotyky, nespouští běžné animace. Není nutné zařízení záměrně resetovat.

## Hardware a konfigurace
Výchozí předpokládané propojení (ověřit podle skutečné desky a současného funkčního zapojení):

| Modul | Vývod | ESP32-C3 |
|---|---|---|
| OLED SH1106 | VCC / GND | 3V3 / GND |
| OLED SH1106 | SDA / SCL | GPIO4 / GPIO5 |
| TTP223 | VCC / GND | 3V3 / GND |
| TTP223 | OUT | GPIO1 |

OLED a případné budoucí I²C moduly sdílejí sběrnici. **Zatím nepřipojuj MPU6050, rotační enkodér ani buzzer.** Fotografie rotačního enkodéru KY-040 není snímač otřesů. Pokud existující firmware používá jiné funkční piny, nejprve je porovnej a upozorni na rozdíl; nepřepisuj je bez ověření.

`platformio.ini` má cílit na `esp32-c3-devkitm-1`, Arduino, U8g2 (pokud je v projektu již používáno) a Serial Monitor 115200. Zachovej aktuálně funkční USB CDC nastavení, zejména pokud je vyžadováno:

```ini
build_flags =
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
```

Neprováděj zbytečný upgrade platformy/frameworku. Pokud projekt používá `src_dir`, respektuj jej.

## Navržená struktura
Uprav dle existujícího repozitáře, ale zachovej oddělení odpovědností:

```text
esp32/
├── platformio.ini
└── src/
    ├── main.cpp
    ├── Robot.h / Robot.cpp
    ├── config/Pins.h
    ├── input/InputController.h / .cpp
    ├── behavior/BehaviourProgram.h / .cpp
    ├── display/OutputDisplay.h / .cpp
    ├── display/AnimationPlayer.h / .cpp
    ├── display/AnimationId.h
    ├── display/animations/
    │   ├── AnimationCatalog.h / .cpp
    │   ├── YawnAnimation.cpp
    │   ├── AngryAnimation.cpp
    │   ├── SmileAnimation.cpp
    │   └── BlinkAnimation.cpp
    └── diagnostics/HardwareDiagnostics.h / .cpp
```

Pokud je existující implementace framebufferu/animací sdílená s PC SDL simulátorem, **upřednostni znovupoužití** a nezdvojuj grafickou logiku. V případě nutnosti uprav strukturu tak, aby se na ESP32 nevázala SDL2.

## Rozhraní a tok událostí
Doporučené veřejné API (můžeš přizpůsobit skutečnému kódu):

```cpp
enum class InputEvent { NONE, TAP, HOLD };
enum class RobotState { IDLE, TIRED, SLEEP, ERROR };
enum class AnimationId { IDLE, BLINK, YAWN, ANGRY, SMILE, SURPRISED, SLEEP, ERROR_FACE };

class InputController {
public:
    void begin();
    void update(uint32_t nowMs);
    bool pollEvent(InputEvent& event); // každá událost se přečte právě jednou
};

class BehaviourProgram {
public:
    void begin();
    void handleEvent(InputEvent event);
    void update(uint32_t nowMs);
    // Požadavek na animaci předá OutputDisplay přes jasné rozhraní.
};

class OutputDisplay {
public:
    bool begin();
    void playAnimation(AnimationId id);
    void update(uint32_t nowMs);
    bool isAnimationFinished() const;
    void showErrorFace();
};
```

Preferuj jednoduché předávání událostí bez dynamických alokací a bez velkých front. `Robot` může vlastnit jednotlivé komponenty a propojovat je; `main` má být minimální. Není potřeba Java-style `ActionListener` ani složitá hierarchie rozhraní.

### Dotykový senzor
- Výchozí předpoklad: TTP223 aktivní v HIGH; pokud má skutečný modul jinou polaritu, nastav ji konfigurovatelně.
- Debounce proti zákmitům, například 30–50 ms.
- Krátký dotyk: `TAP` **až při uvolnění**, pokud délka nepřekročila práh.
- Dlouhé podržení: `HOLD` **jednou okamžitě po dosažení prahu** (výchozí 1000 ms), nikoli až při uvolnění.
- Po `HOLD` už uvolnění **nesmí** vytvořit `TAP`.
- Bez blokujícího čekání; časování přes `millis()`.
- Události vypisuj do Serial pro testování.

### Chování – první funkční verze
Implementuj jednoduchý, snadno rozšiřitelný stavový automat. `IDLE`, `TIRED`, `SLEEP` jsou dlouhodobé stavy; `ERROR` je systémový režim s nejvyšší prioritou. `ANGRY`, `YAWN`, `SMILE` apod. jsou **krátkodobé animace, ne dlouhodobé stavy**.

Pro první test bez hodin a bez složitého rozvrhu:
- Po úspěšném startu stav `IDLE`, přehrávat klidný obličej a občas mrknout.
- `TAP` v `IDLE`: přehrát `ANGRY`, potom se vrátit k `IDLE`.
- `HOLD` v `IDLE`: přehrát `SMILE`, potom se vrátit k `IDLE`.
- Udržet rozhraní pro budoucí `TIRED`, `SLEEP`, `YAWN` a reakce na pohyb; neimplementovat nyní neodsouhlasené časové přechody.
- Rozhodnutí, co následuje po dokončení animace, dělá `BehaviourProgram`; `OutputDisplay` pouze hlásí dokončení.
- Vyřešit prioritu nových událostí při probíhající animaci (pro první verzi může TAP/HOLD přerušit běžnou animaci, ERROR přeruší vše).
- Zabránit tomu, aby se při každém `loop()` restartovala stejná animace.

### Animace a displej
- Hardcoded definice animací udržuj v `animations/` a vybírej je podle `AnimationId` v `AnimationCatalog`.
- `OutputDisplay::playAnimation(id)` zahájí přehrávání; `update(nowMs)` posune snímek, pokud nastal čas, a vykreslí ho přes U8g2 nebo existující framebuffer.
- Žádné `delay()` v přehrávači, žádné dynamické vytváření velkých bitmap na každém snímku.
- Preferuj programatické kreslení očí a geometrických tvarů, protože OLED má 128×64 a je monochromatický.
- Udržuj definice pokud možno v `const`/`constexpr`, respektuj paměťové limity ESP32-C3.
- Pokud v projektu existují hotové animace, použij je; jinak vytvoř minimální funkční sadu `IDLE`, `BLINK`, `ANGRY`, `SMILE`, `YAWN`, `ERROR_FACE` s možností pozdější výměny.
- `ERROR_FACE`: **statický smutný obličej a čitelný nápis `ERROR`**. Při rozlišení 128×64 nesmí text zakrývat oči.

## Kontrola hardwaru při každém startu
`HardwareDiagnostics` provede startup check **před** aktivací běžného chování:
1. Spustí Serial na 115200 a I²C (`Wire.begin(SDA, SCL)`).
2. Zkontroluje odpověď OLED na očekávané I²C adrese (typicky `0x3C`, případně `0x3D`) a úspěšnost inicializace displeje v rámci možností knihovny.
3. Nastaví GPIO dotykového senzoru a provede pouze smysluplné kontroly. **TTP223 má jen digitální OUT: nelze spolehlivě detekovat jeho přítomnost nebo správnou funkci pouhým čtením pinu.** Neoznačuj ho automaticky jako OK/FAILED podle jediné hodnoty HIGH/LOW. V logu rozliš `CONFIGURED` od `VERIFIED`.
4. Vypíše přehled `[CHECK] OLED: OK/FAILED`, `[CHECK] TOUCH: CONFIGURED (not electrically verifiable)`.
5. Při kritické chybě přejde do `ERROR`, bez běžných animací a bez reakcí na dotyk.

**Omezení:** Když OLED fyzicky nefunguje, smutný obličej ani text `ERROR` na něm nelze zobrazit. Vypiš důvod na Serial a zůstaň v bezpečném chybovém režimu. Nefinguj úspěšný test TTP223. Pokud je žádoucí přísnější ověření dotyku, lze později přidat uživatelský test vyžadující dotyk při startu, ale **nyní neblokuj boot čekáním na dotyk**.

## Diagnostické logy
Příklad (skutečný text může být jiný):

```text
[BOOT] DesktopRobot
[CHECK] OLED 0x3C: OK
[CHECK] TOUCH GPIO1: CONFIGURED (not verified)
[STATE] IDLE
[TOUCH] pressed
[EVENT] TAP
[ANIM] ANGRY
[ANIM] finished
[STATE] IDLE
```

Při chybě:

```text
[CHECK] OLED: FAILED
[ERROR] Display not responding
[STATE] ERROR
```

Logy nesmějí zahlcovat konzoli každý průchod `loop()`.

## Kvalita implementace
- Kompatibilita s PlatformIO, Arduino frameworkem a **ESP32-C3 Super Mini**.
- Bez blokujících `delay()` v běžném běhu.
- Vyhni se zbytečnému `new`, dynamickým alokacím, přerostlým abstrakcím a cyklickým závislostem.
- Používej jednotné názvosloví a oprav překlepy (`InputController`, `BehaviourProgram`, `OutputDisplay`).
- `Robot::begin()` vrací nebo ukládá stav diagnostiky; `Robot::update()` buď vykresluje error, nebo zpracovává vstupy, chování a výstup.
- Komentáře jen tam, kde vysvětlují neobvyklé rozhodnutí.
- Nezasahuj do PC simulátoru, pokud to není nutné pro sdílení existujících animací.
- Přidej stručné README: struktura, zapojení, jak sestavit, nahrát a otevřít Serial Monitor, známá omezení diagnostiky.

## Akceptační testy
1. `pio run -e esp32c3` dokončí Build bez chyby.
2. Po nahrání a zapnutí se na funkčním OLED zobrazí klidný obličej.
3. Krátký dotyk → právě jedna událost TAP → animace ANGRY → návrat k IDLE.
4. Podržení déle než 1 s → právě jedna událost HOLD **ještě během držení** → SMILE → IDLE; při puštění už žádný TAP.
5. Během animace se program nezablokuje a dokáže reagovat na další událost podle nastavených priorit.
6. Při odpojeném OLED se firmware nepokouší spustit běžné chování, vypíše chybu na Serial a bezpečně setrvá v ERROR.
7. `BehaviourProgram` neobsahuje definice snímků, bitmap ani vykreslovací kód.
8. `OutputDisplay` nerozhoduje o emocích ani o výběru následující animace.
9. Každý restart spouští diagnostiku.

## Postup práce pro implementačního agenta
1. Nejdřív prohlédni současnou strukturu `esp32/`, `platformio.ini`, existující OLED/animace a případné sdílené zdroje simulátoru.
2. Napiš krátký plán, které soubory zachováš, upravíš a vytvoříš.
3. Implementuj moduly a propojení bez zbytečného refaktoringu ostatních částí.
4. Proveď Build, oprav chyby a uveď výsledek. Pokud nemáš přístup k připojenému hardwaru, **netvrď, že fyzické testy proběhly**.
5. Na závěr vypiš změněné soubory, stručně popiš datový tok a uveď přesný postup ověření na ESP32.

**Neimplementuj nyní:** buzzer/zvuky, rotační enkodér KY-040, MPU6050, Wi‑Fi/NTP, automatické denní usínání/probouzení ani složitý náhodný repertoár. Architektura však musí umožnit jejich pozdější doplnění.
