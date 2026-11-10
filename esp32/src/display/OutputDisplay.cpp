#include "OutputDisplay.h"
#include "animations/AnimationCatalog.h"
#include "animations/PowerOnAnimation.h"
#include <Arduino.h>
#include <Wire.h>
#include <cstdio>
#include <cstring>
namespace {
// A compact, two-frame Wi-Fi mark. AnimationPlayer restores the face loop when
// this one-shot finishes, so connection feedback never changes behavior state.
constexpr desktoprobot::DrawCommand wifiSmall[] = {
 desktoprobot::line(106,10,110,7), desktoprobot::line(110,7,114,10),
 desktoprobot::line(109,13,111,11), desktoprobot::pixel(110,16)
};
constexpr desktoprobot::DrawCommand wifiLarge[] = {
 desktoprobot::line(99,12,110,3), desktoprobot::line(110,3,121,12),
 desktoprobot::line(103,15,110,9), desktoprobot::line(110,9,117,15),
 desktoprobot::line(107,17,110,14), desktoprobot::line(110,14,113,17),
 desktoprobot::pixel(110,21)
};
constexpr desktoprobot::Frame wifiFrames[] = {{180,wifiSmall,4},{700,wifiLarge,7}};
constexpr desktoprobot::Animation wifiAnimation{"WIFI CONNECTED",wifiFrames,2,false};
constexpr desktoprobot::DrawCommand wifiFailed[] = {
 desktoprobot::line(99,12,110,3), desktoprobot::line(110,3,121,12),
 desktoprobot::line(103,15,110,9), desktoprobot::line(110,9,117,15),
 desktoprobot::line(107,17,110,14), desktoprobot::line(110,14,113,17),
 desktoprobot::pixel(110,21),
 desktoprobot::line(99,2,121,22), desktoprobot::line(121,2,99,22)
};
constexpr desktoprobot::Frame wifiFailedFrames[] = {{900,wifiFailed,9}};
constexpr desktoprobot::Animation wifiFailedAnimation{"WIFI FAILED",wifiFailedFrames,1,false};
}
bool OutputDisplay::begin() {
 display_.setI2CAddress(Pins::OLED_ADDRESS<<1);
 display_.setBusClock(Pins::I2C_HZ);
 // U8g2 begin() cannot verify the physical display; diagnostics also checks ACK.
 ready_=display_.begin();
 if (ready_) {
  display_.setContrast(80);
  // Clear retained OLED RAM before clocks, sound or any face can be started.
  framebuffer_.clear(); display_.clearBuffer(); display_.sendBuffer();
 }
 return ready_;
}
void OutputDisplay::playPowerOn(uint32_t now) {
 play(desktoprobot::getPowerOnAnimation(),now);
 update(now); // Send the blank opening frame immediately, before the first loop.
}
void OutputDisplay::playWifiConnected(uint32_t now) {
 if (!ready_ || error_) return;
 wifiOverlay_=&wifiAnimation; wifiOverlayStart_=now;
 wifiOverlayFrame_=static_cast<std::size_t>(-1); dirty_=true;
 report("[ANIM] started WIFI CONNECTED");
}
void OutputDisplay::playWifiFailed(uint32_t now) {
 if (!ready_ || error_) return;
 wifiOverlay_=&wifiFailedAnimation; wifiOverlayStart_=now;
 wifiOverlayFrame_=static_cast<std::size_t>(-1); dirty_=true;
 report("[ANIM] started WIFI FAILED");
}
void OutputDisplay::setTestClock(const LocalTime& time) {
 char next[sizeof(clockText_)];
 if (time.valid && time.second<86400) {
  const unsigned hour=time.second/3600, minute=(time.second/60)%60, second=time.second%60;
  next[0]='0'+hour/10; next[1]='0'+hour%10; next[2]=':';
  next[3]='0'+minute/10; next[4]='0'+minute%10; next[5]=':';
  next[6]='0'+second/10; next[7]='0'+second%10; next[8]='\0';
 } else std::memcpy(next,"--:--:--",sizeof(next));
 if (std::strcmp(next,clockText_)==0) return;
 std::memcpy(clockText_,next,sizeof(clockText_));
 dirty_=true;
}
void OutputDisplay::setClockVisible(bool visible) {
 if (clockVisible_==visible) return;
 clockVisible_=visible;
 dirty_=true;
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
 if (wifiOverlay_ && wifiOverlayFrame_ < wifiOverlay_->frameCount)
  desktoprobot::drawFrame(wifiOverlay_->frames[wifiOverlayFrame_],framebuffer_);
 display_.clearBuffer();
 // Both buffers use vertical 8-pixel pages in the same 128x64 layout.
 memcpy(display_.getBufferPtr(),framebuffer_.data().data(),desktoprobot::Framebuffer::BufferSize);
 if (clockVisible_ && (!wifiOverlay_ || wifiOverlayFrame_>=wifiOverlay_->frameCount)) {
  display_.setFont(u8g2_font_6x12_tf);
  display_.drawStr(79,11,clockText_);
 }
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
 if (wifiOverlay_) {
  uint32_t elapsed=now-wifiOverlayStart_;
  std::size_t frame=0;
  while (frame<wifiOverlay_->frameCount && elapsed>=wifiOverlay_->frames[frame].durationMs) {
   elapsed-=wifiOverlay_->frames[frame].durationMs; ++frame;
  }
  if (frame>=wifiOverlay_->frameCount) {
   wifiOverlay_=nullptr; wifiOverlayFrame_=static_cast<std::size_t>(-1); dirty_=true;
  } else if (frame!=wifiOverlayFrame_) { wifiOverlayFrame_=frame; dirty_=true; }
 }
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
