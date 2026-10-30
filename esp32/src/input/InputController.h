#pragma once
#include <cstdint>
enum class InputEvent { NONE, TAP, HOLD };
class InputController {
public:
 void begin();
 void update(uint32_t nowMs);
 bool pollEvent(InputEvent& event);
private:
 bool readTouch() const;
 bool raw_ = false, stable_ = false, held_ = false;
 uint32_t changedAt_ = 0, pressedAt_ = 0;
 InputEvent pending_ = InputEvent::NONE;
};
