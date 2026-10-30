#pragma once
#include <U8g2lib.h>
#include "AnimationPlayer.h"
#include "AnimationId.h"
#include "../config/Pins.h"
class OutputDisplay {
public:
 bool begin();
 void playAnimation(AnimationId id);
 void update(uint32_t nowMs);
 bool isAnimationFinished() const { return player_.isFinished(); }
 void showErrorFace();
private:
 U8G2_SH1106_128X64_NONAME_F_HW_I2C display_{U8G2_R0,U8X8_PIN_NONE,Pins::SCL,Pins::SDA};
 desktoprobot::Framebuffer framebuffer_;
 AnimationPlayer player_;
 bool ready_=false, dirty_=false, error_=false, completionLogged_=false;
 void render();
};
