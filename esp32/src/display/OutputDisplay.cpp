#include "OutputDisplay.h"
#include "animations/AnimationCatalog.h"
#include <Arduino.h>
#include <Wire.h>
bool OutputDisplay::begin() {
 display_.setI2CAddress(Pins::OLED_ADDRESS<<1);
 display_.setBusClock(Pins::I2C_HZ);
 // U8g2 begin() cannot verify the physical display; diagnostics also checks ACK.
 ready_=display_.begin();
 return ready_;
}
void OutputDisplay::playAnimation(AnimationId id) {
 if (!ready_ || error_) return;
 const auto& animation=getAnimation(id);
 player_.play(animation,millis()); dirty_=true; completionLogged_=false;
 Serial.printf("[ANIM] %s\n",animation.name);
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
 dirty_=player_.update(now) || dirty_;
 if (player_.isFinished() && !completionLogged_) {
  Serial.println("[ANIM] finished"); completionLogged_=true;
 }
 if (dirty_) render();
}
void OutputDisplay::showErrorFace() {
 if (!ready_ || error_) return;
 player_.play(getAnimation(AnimationId::ERROR_FACE),millis());
 error_=true; render();
}
