#include "BehaviourProgram.h"
#include <Arduino.h>
void BehaviourProgram::begin() { setState(RobotState::IDLE); }
void BehaviourProgram::playBase() {
 transient_=false; idleSince_=millis();
 switch(state_) {
 case RobotState::IDLE: display_.playAnimation(AnimationId::IDLE); break;
 case RobotState::TIRED: display_.playAnimation(AnimationId::TIRED); break;
 case RobotState::SLEEP: display_.playAnimation(AnimationId::SLEEP); break;
 case RobotState::ERROR: display_.showErrorFace(); break;
 }
}
void BehaviourProgram::setState(RobotState state) {
 if (state_==RobotState::ERROR) return; // Error is latched until restart.
 state_=state;
 const char* names[]={"IDLE","TIRED","SLEEP","ERROR"};
 Serial.printf("[STATE] %s\n",names[static_cast<unsigned>(state)]);
 playBase();
}
void BehaviourProgram::handleEvent(InputEvent event) {
 if (state_!=RobotState::IDLE || event==InputEvent::NONE) return;
 // New input interrupts any normal one-shot, including the same expression.
 display_.playAnimation(event==InputEvent::HOLD ? AnimationId::SMILE : AnimationId::ANGRY);
 transient_=true;
}
void BehaviourProgram::update(uint32_t now) {
 if (state_==RobotState::ERROR) return;
 if (transient_) {
  if (display_.isAnimationFinished()) playBase();
 } else if (state_==RobotState::IDLE && now-idleSince_>=4000) {
  display_.playAnimation(AnimationId::BLINK); transient_=true;
 }
}
