#include "OutputDisplay.h"
#include "animations/AnimationCatalog.h"
#include "animations/PowerOnAnimation.h"
#include <Arduino.h>
#include <Wire.h>
#include <cstdio>
bool OutputDisplay::begin() {
 display_.setI2CAddress(Pins::OLED_ADDRESS<<1);
 display_.setBusClock(Pins::I2C_HZ);
 // U8g2 begin() cannot verify the physical display; diagnostics also checks ACK.
 ready_=display_.begin();
 if (ready_) {
  // Clear retained OLED RAM before clocks, sound or any face can be started.
  framebuffer_.clear(); display_.clearBuffer(); display_.sendBuffer();
 }
 return ready_;
}
void OutputDisplay::playPowerOn(uint32_t now) {
 play(desktoprobot::getPowerOnAnimation(),now);
 update(now); // Send the blank opening frame immediately, before the first loop.
}
void OutputDisplay::play(const desktoprobot::Animation& animation, uint32_t now, std::size_t limit) {
 if (!ready_ || error_) return;
 player_.play(animation, limit); lastUpdate_=now; dirty_=true; completionLogged_=false;
 char message[96]; std::snprintf(message,sizeof(message),"[ANIM] started %s",animation.name); report(message);
}
void OutputDisplay::playAnimation(AnimationId id, uint32_t now, bool holdExpression) {
 if (!ready_ || error_) return;
 const std::size_t limit = holdExpression ? desktoprobot::getExpressionFrameCount(id) : 0;
 play(getAnimation(id), now, limit);
}
void OutputDisplay::playBase(RobotState state, uint32_t now) {
 if (!ready_ || error_) return;
 play(desktoprobot::getStateAnimation(static_cast<desktoprobot::RobotState>(state)), now);
}
void OutputDisplay::playShakeRecovery(uint32_t now) {
 if (!ready_ || error_) return;
 // Capture recovery before replacing the catalog buffer: it starts at the displayed frame.
 const auto& recovery=desktoprobot::getShakeRecoveryAnimation(player_.frame());
 play(recovery, now);
}
void OutputDisplay::playTouchReturn(uint32_t now) {
 if (ready_ && !error_) play(desktoprobot::getTouchReturnAnimation(player_.frame()),now);
}
void OutputDisplay::render() {
 framebuffer_.clear(); desktoprobot::drawFrame(player_.frame(),framebuffer_);
 display_.clearBuffer();
 // Both buffers use vertical 8-pixel pages in the same 128x64 layout.
 memcpy(display_.getBufferPtr(),framebuffer_.data().data(),desktoprobot::Framebuffer::BufferSize);
 if (error_) {
  display_.setFont(u8g2_font_6x12_tf);
  display_.drawStr(49,62,"ERROR");
 }
 display_.sendBuffer(); dirty_=false;
}
void OutputDisplay::update(uint32_t now) {
 if (!ready_ || error_) return;
 const auto* previous=&player_.frame();
 player_.update(now-lastUpdate_); lastUpdate_=now;
 dirty_=previous!=&player_.frame() || dirty_;
 if (player_.isFinished() && !completionLogged_) {
  char message[96]; std::snprintf(message,sizeof(message),"[ANIM] finished %s",player_.getCurrentAnimation()->name); report(message); completionLogged_=true;
 }
 if (dirty_) render();
}
void OutputDisplay::showErrorFace() {
 if (!ready_ || error_) return;
 player_.play(getErrorAnimation());
 error_=true; render();
}

void OutputDisplay::report(const char* message) {
 if (log_) log_(logContext_,message);
 else Serial.println(message);
}

void OutputDisplay::playTouchWake(RobotState source,uint32_t now) { if(ready_ && !error_) play(getTouchWakeAnimation(static_cast<desktoprobot::RobotState>(source)),now); }
void OutputDisplay::playTouchLookUp(RobotState source,uint32_t now) { if(ready_ && !error_) play(getTouchWakeAnimation(static_cast<desktoprobot::RobotState>(source),true),now); }
