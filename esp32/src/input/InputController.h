#pragma once
#include <cstdint>
#include "../behavior/BehaviourProgram.h"
#include "MotionSensor.h"
class InputController {
public:
 void begin();
 bool beginMotionSensor();
 void update(uint32_t nowMs);
 bool pollEvent(InputEvent& event);
 // Edges remain pending even while an OLED transfer blocks the main loop.
 bool touchPending() const { return raw_ || stable_ || edgeRead_ != edgeWrite_ || eventRead_ != eventWrite_; }
private:
 static constexpr unsigned QueueSize = 32;
 struct Edge { uint32_t at; bool pressed; };
 Edge edges_[QueueSize]{};
 InputEvent events_[QueueSize]{};
 volatile unsigned edgeRead_ = 0, edgeWrite_ = 0;
 unsigned eventRead_ = 0, eventWrite_ = 0;
 static void onEdge(void* context);
 void settle(uint32_t now);
 void emit(InputEvent event);
 bool readTouch() const;
 MotionSensor motion_;
 bool raw_ = false, stable_ = false, held_ = false;
 uint32_t changedAt_ = 0, pressedAt_ = 0;
};
