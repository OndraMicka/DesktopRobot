#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <initializer_list>

namespace
{
    constexpr uint8_t PIN_SDA = 4;
    constexpr uint8_t PIN_SCL = 5;
    constexpr uint8_t PIN_TOUCH = 1;
    constexpr bool TOUCH_ACTIVE_HIGH = true;
    constexpr uint32_t DEBOUNCE_MS = 35;
    constexpr uint32_t HOLD_MS = 700;
    constexpr uint32_t DISPLAY_REFRESH_MS = 50;

    // Explicit pins also keep U8g2 initialization on the scanned I2C bus.
    U8G2_SH1106_128X64_NONAME_F_HW_I2C display(
        U8G2_R0, U8X8_PIN_NONE, PIN_SCL, PIN_SDA);

    enum class Face { Neutral, Happy, Surprised };
    bool oledFound = false;
    bool stableTouch = false;
    bool lastRawTouch = false;
    bool holdReported = false;
    uint32_t lastRawChange = 0;
    uint32_t touchStart = 0;
    uint32_t lastFrame = 0;
    uint32_t faceStart = 0;
    uint32_t faceDuration = 0;
    Face face = Face::Neutral;

    bool readTouch()
    {
        const bool high = digitalRead(PIN_TOUCH) == HIGH;
        return TOUCH_ACTIVE_HIGH ? high : !high;
    }

    void showFace(Face expression, uint32_t now, uint32_t duration)
    {
        face = expression;
        faceStart = now;
        faceDuration = duration;
    }

    void reportHold(uint32_t now)
    {
        holdReported = true;
        Serial.println("[EVENT] HOLD");
        showFace(Face::Happy, now, 1200);
    }

    void scanI2C()
    {
        Serial.println("[I2C] Scanning...");
        uint8_t count = 0;
        for (uint8_t address = 1; address < 127; ++address)
        {
            Wire.beginTransmission(address);
            if (Wire.endTransmission() != 0) continue;
            Serial.printf("[I2C] Device found: 0x%02X\n", address);
            ++count;
            if (!oledFound && (address == 0x3C || address == 0x3D))
            {
                oledFound = true;
                display.setI2CAddress(address << 1); // U8g2 uses an 8-bit address.
            }
        }
        if (count == 0) Serial.println("[I2C] No devices found");
        Serial.printf("[OLED] %s\n", oledFound ? "Detected" : "Not detected (0x3C/0x3D)");
    }

    void updateTouch(uint32_t now)
    {
        const bool raw = readTouch();
        if (raw != lastRawTouch)
        {
            lastRawTouch = raw;
            lastRawChange = now;
        }
        if (raw != stableTouch && now - lastRawChange >= DEBOUNCE_MS)
        {
            stableTouch = raw;
            if (stableTouch)
            {
                touchStart = now;
                holdReported = false;
                Serial.println("[TOUCH] Pressed");
            }
            else
            {
                const uint32_t duration = now - touchStart;
                Serial.printf("[TOUCH] Released after %lu ms\n", static_cast<unsigned long>(duration));
                // Also classify a long press correctly if a slow frame skipped its threshold.
                if (!holdReported && duration >= HOLD_MS) reportHold(now);
                if (!holdReported)
                {
                    Serial.println("[EVENT] TAP");
                    showFace(Face::Surprised, now, 900);
                }
                else
                {
                    // Keep the happy expression briefly after release, even for a long hold.
                    showFace(Face::Happy, now, 1200);
                }
            }
        }
        if (stableTouch && raw && !holdReported && now - touchStart >= HOLD_MS)
            reportHold(now);
    }

    void drawFace(uint32_t now)
    {
        if (!oledFound || now - lastFrame < DISPLAY_REFRESH_MS) return;
        lastFrame = now;
        // Unsigned elapsed time remains valid when millis() wraps around.
        if (!stableTouch && face != Face::Neutral && now - faceStart >= faceDuration)
            face = Face::Neutral;

        display.clearBuffer();
        if (face == Face::Surprised)
        {
            display.drawCircle(40, 25, 13);
            display.drawCircle(88, 25, 13);
            display.drawDisc(40, 25, 5);
            display.drawDisc(88, 25, 5);
            display.drawCircle(64, 51, 6);
        }
        else if (face == Face::Happy)
        {
            // Thick, upward-curved closed eyes; no version-dependent arc API.
            for (uint8_t centerX : { uint8_t{34}, uint8_t{94} })
            {
                display.drawLine(centerX - 13, 28, centerX - 6, 22);
                display.drawLine(centerX - 6, 22, centerX + 6, 22);
                display.drawLine(centerX + 6, 22, centerX + 13, 28);
                display.drawLine(centerX - 13, 29, centerX - 6, 23);
                display.drawLine(centerX - 6, 23, centerX + 6, 23);
                display.drawLine(centerX + 6, 23, centerX + 13, 29);
            }
            display.drawLine(48, 46, 56, 52);
            display.drawLine(56, 52, 72, 52);
            display.drawLine(72, 52, 80, 46);
        }
        else
        {
            display.drawRBox(18, 11, 32, 28, 12);
            display.drawRBox(78, 11, 32, 28, 12);
            display.drawRBox(54, 49, 20, 5, 2);
        }
        display.sendBuffer();
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1200); // Startup only: give USB CDC time to enumerate.
    Serial.println("\n[BOOT] DesktopRobot hardware test");
    Serial.printf("[PINS] OLED SDA=%u SCL=%u; TTP223 OUT=%u\n", PIN_SDA, PIN_SCL, PIN_TOUCH);

    pinMode(PIN_TOUCH, INPUT);
    lastRawTouch = stableTouch = readTouch();
    lastRawChange = touchStart = millis();
    Serial.printf("[TOUCH] Initial state: %s\n", stableTouch ? "PRESSED" : "RELEASED");

    if (Wire.begin(PIN_SDA, PIN_SCL))
    {
        Wire.setClock(100000);
        Wire.setTimeOut(50);
        scanI2C();
    }
    else
    {
        Serial.println("[I2C] Initialization failed");
    }
    if (oledFound)
    {
        display.setBusClock(100000);
        display.begin();
        Serial.println("[OLED] Initialized");
        drawFace(millis());
    }
    else
    {
        Serial.println("[OLED] Skipping display init; check wiring");
    }
    Serial.println("[READY] Touch sensor test running");
}

void loop()
{
    const uint32_t now = millis();
    updateTouch(now);
    drawFace(now);
    delay(1); // Yield to USB/RTOS; touch and display timing use millis().
}
