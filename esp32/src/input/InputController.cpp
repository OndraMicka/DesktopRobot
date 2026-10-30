#include "InputController.h"
#include "../config/Pins.h"
#include <Arduino.h>
bool InputController::readTouch() const { return (digitalRead(Pins::TOUCH)==HIGH)==Pins::TOUCH_ACTIVE_HIGH; }
void InputController::begin() {
 pinMode(Pins::TOUCH,INPUT);
 raw_=stable_=readTouch(); held_=false; pending_=InputEvent::NONE;
 changedAt_=pressedAt_=millis();
}
void InputController::update(uint32_t now) {
 const bool sample=readTouch();
 if (sample!=raw_) { raw_=sample; changedAt_=now; }
 if (raw_!=stable_ && now-changedAt_>=Pins::DEBOUNCE_MS) {
  stable_=raw_;
  if (stable_) {
   pressedAt_=changedAt_; held_=false;
   Serial.println("[TOUCH] pressed");
  } else {
   Serial.println("[TOUCH] released");
   if (!held_) {
    pending_ = changedAt_-pressedAt_ >= Pins::HOLD_MS ? InputEvent::HOLD : InputEvent::TAP;
    Serial.println(pending_==InputEvent::HOLD ? "[EVENT] HOLD" : "[EVENT] TAP");
   }
  }
 }
 if (stable_ && raw_ && !held_ && now-pressedAt_>=Pins::HOLD_MS) {
  held_=true; pending_=InputEvent::HOLD; Serial.println("[EVENT] HOLD");
 }
}
bool InputController::pollEvent(InputEvent& event) {
 event=pending_; pending_=InputEvent::NONE; return event!=InputEvent::NONE;
}
