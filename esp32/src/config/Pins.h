#pragma once
#include <cstdint>
namespace Pins {
constexpr uint8_t SDA = 4, SCL = 5, TOUCH = 1;
constexpr uint8_t BUZZER = 3; // Control a verified piezo driver; GPIO is 3.3 V.
constexpr bool TOUCH_ACTIVE_HIGH = true;
constexpr uint8_t OLED_ADDRESS = 0x3C; // Change explicitly for a 0x3D module.
constexpr uint32_t I2C_HZ = 100000, DEBOUNCE_MS = 35, HOLD_MS = 700;
}
