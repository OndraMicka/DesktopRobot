# DesktopRobot – první test hardwaru

## Cíl

Otestovat **ESP32-C3 Super Mini**, **OLED 128×64 SH1106 (I²C)** a **kapacitní dotykový senzor TTP223** na breadboardu. Bez bzučáku a bez senzoru otřesů. V Serial Monitoru se vypisují výsledky inicializace, změny dotyku a rozpoznané krátké/dlouhé stisknutí. Na OLED se vykreslí jednoduchý obličej, který na dotyk reaguje.

> **Pozor:** Zapojení na fotografii nelze zcela ověřit. Před připojením USB zkontroluj skutečné popisky pinů na všech deskách. Následující kód předpokládá zapojení uvedené níže. Modul KY-040 z předchozí fotografie je rotační enkodér, nikoliv senzor otřesů, a v tomto testu se nepoužívá.

## Zapojení

| Modul | Pin modulu | ESP32-C3 Super Mini |
|---|---|---|
| OLED SH1106 | VCC | 3V3 |
| OLED SH1106 | GND | GND |
| OLED SH1106 | SDA | GPIO4 |
| OLED SH1106 | SCL | GPIO5 |
| TTP223 | VCC | 3V3 |
| TTP223 | GND | GND |
| TTP223 | OUT / I/O | GPIO1 |

- Oba moduly napájej **3,3 V** a propojuj společnou zem **GND**.
- OLED a dotykový modul nejsou propojené datově navzájem; oba komunikují s ESP32.
- U některých verzí OLED se liší pořadí vývodů. Řiď se nápisy na vlastní desce.
- TTP223 je zde předpokládán v běžném režimu: výstup **HIGH při dotyku**, **LOW bez dotyku**. Pokud je modul přepnut do jiného režimu, uprav konstantu `TOUCH_ACTIVE_HIGH`.

## Prostředí

- Arduino IDE nebo PlatformIO s podporou **ESP32-C3**.
- Nainstaluj knihovnu **U8g2** (autor olikraus) přes správce knihoven.
- V Arduino IDE vyber vhodnou desku ESP32-C3 (např. `ESP32C3 Dev Module`) a příslušný port.
- V nastavení desky může být pro USB Serial Monitor potřeba **USB CDC On Boot: Enabled** (podle verze balíčku ESP32).
- Serial Monitor nastav na **115200 baud**.

## Kompletní testovací program (`hardware_test.ino`)

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// Zapojení podle tabulky výše.
constexpr uint8_t PIN_SDA = 4;
constexpr uint8_t PIN_SCL = 5;
constexpr uint8_t PIN_TOUCH = 1;
constexpr bool TOUCH_ACTIVE_HIGH = true;

constexpr unsigned long DEBOUNCE_MS = 35;
constexpr unsigned long HOLD_MS = 700;
constexpr unsigned long DISPLAY_REFRESH_MS = 50;

// SH1106, 128x64, I2C, hardwarové I2C; celá obrazovka v RAM.
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

bool oledFound = false;
bool stableTouch = false;
bool lastRawTouch = false;
bool holdReported = false;
unsigned long lastRawChange = 0;
unsigned long touchStart = 0;
unsigned long lastFrame = 0;

// 0 = klid, 1 = radost, 2 = překvapení
uint8_t face = 0;
unsigned long faceUntil = 0;

bool readTouch() {
  bool level = digitalRead(PIN_TOUCH) == HIGH;
  return TOUCH_ACTIVE_HIGH ? level : !level;
}

void scanI2C() {
  Serial.println("[I2C] Scanning...");
  uint8_t count = 0;
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("[I2C] Device found: 0x%02X\n", address);
      ++count;
      if (address == 0x3C || address == 0x3D) {
        oledFound = true;
        // U8g2 vyžaduje 8bitovou I2C adresu.
        display.setI2CAddress(address << 1);
      }
    }
  }
  if (count == 0) Serial.println("[I2C] No devices found");
  Serial.printf("[OLED] %s\n", oledFound ? "Detected" : "Not detected (0x3C/0x3D)");
}

void updateTouch(unsigned long now) {
  const bool raw = readTouch();
  if (raw != lastRawTouch) {
    lastRawTouch = raw;
    lastRawChange = now;
  }

  if (raw != stableTouch && now - lastRawChange >= DEBOUNCE_MS) {
    stableTouch = raw;
    if (stableTouch) {
      touchStart = now;
      holdReported = false;
      Serial.println("[TOUCH] Pressed");
    } else {
      const unsigned long duration = now - touchStart;
      Serial.printf("[TOUCH] Released after %lu ms\n", duration);
      if (!holdReported) {
        Serial.println("[EVENT] TAP");
        face = 2;
        faceUntil = now + 900;
      }
    }
  }

  if (stableTouch && !holdReported && now - touchStart >= HOLD_MS) {
    holdReported = true;
    Serial.println("[EVENT] HOLD");
    face = 1;
    faceUntil = now + 1200;
  }
}

void drawFace(unsigned long now) {
  if (!oledFound) return;
  if (now - lastFrame < DISPLAY_REFRESH_MS) return;
  lastFrame = now;

  if (!stableTouch && face != 0 && (long)(now - faceUntil) >= 0) {
    face = 0;
  }

  display.clearBuffer();
  if (face == 2) {
    // Překvapení po krátkém dotyku.
    display.drawCircle(40, 30, 13);
    display.drawCircle(88, 30, 13);
    display.drawDisc(40, 30, 5);
    display.drawDisc(88, 30, 5);
  } else if (face == 1) {
    // Radost po dlouhém podržení.
    display.drawArc(25, 32, 15, 15, 20, 160);
    display.drawArc(73, 32, 15, 15, 20, 160);
    display.drawLine(46, 49, 82, 49);
  } else {
    // Klidový obličej.
    display.drawRBox(25, 17, 29, 34, 10);
    display.drawRBox(74, 17, 29, 34, 10);
  }
  display.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  delay(1200); // Pouze při startu, pro otevření USB Serial Monitoru.
  Serial.println("\n[BOOT] DesktopRobot hardware test");

  pinMode(PIN_TOUCH, INPUT);
  lastRawTouch = readTouch();
  stableTouch = lastRawTouch;
  lastRawChange = millis();
  if (stableTouch) touchStart = millis();
  Serial.printf("[TOUCH] Initial state: %s\n", stableTouch ? "PRESSED" : "RELEASED");

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
  scanI2C();

  if (oledFound) {
    display.begin();
    Serial.println("[OLED] Initialized");
  } else {
    Serial.println("[OLED] Skipping display init; check wiring");
  }
  Serial.println("[READY] Touch sensor test running");
}

void loop() {
  const unsigned long now = millis();
  updateTouch(now);
  drawFace(now);
}
```

## Očekávaný výstup

```text
[BOOT] DesktopRobot hardware test
[TOUCH] Initial state: RELEASED
[I2C] Scanning...
[I2C] Device found: 0x3C
[OLED] Detected
[OLED] Initialized
[READY] Touch sensor test running
[TOUCH] Pressed
[TOUCH] Released after 182 ms
[EVENT] TAP
[TOUCH] Pressed
[EVENT] HOLD
[TOUCH] Released after 1064 ms
```

## Co zkontrolovat, když to nefunguje

1. **Nevidíš nic v Serial Monitoru:** zkontroluj port, 115200 baud, USB datový kabel a nastavení USB CDC On Boot.
2. **`[I2C] No devices found`:** ověř napájení OLED, společnou GND, SDA na GPIO4 a SCL na GPIO5. Neprohazuj SDA a SCL.
3. **OLED je nalezený, ale nic nezobrazuje:** ověř, že jde skutečně o řadič SH1106, ne SSD1306. Pokud máš SSD1306, musí se změnit konstruktor knihovny.
4. **Dotyk nereaguje:** ověř OUT→GPIO1, VCC→3V3 a GND. Zkus nastavit `TOUCH_ACTIVE_HIGH = false` jen pokud tvůj modul skutečně pracuje v invertovaném režimu.
5. **Dotyk se chová náhodně:** zkontroluj pevné propojení GND, kabely a zda nejsou některé piny na breadboardu omylem zkratované.

## Další krok

Jakmile projde OLED a dotyk, můžeme přidat skutečný senzor otřesů/náklonu. **Rotační enkodér KY-040 otřesy neměří**, proto ho nelze použít jako náhradu MPU6050.
