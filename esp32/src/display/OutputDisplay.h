#pragma once
#include <U8g2lib.h>
#include "AnimationPlayer.h"
#include "../behavior/BehaviourProgram.h"
#include "../config/Pins.h"
class OutputDisplay : public AnimationOutput {
public:
 using Log = void (*)(void*, const char*);
 void setLog(Log callback, void* context) { log_=callback; logContext_=context; }
 bool begin();
 void playPowerOn(uint32_t now);
 void playWifiConnected(uint32_t now);
 void playWifiFailed(uint32_t now);
 void playAnimation(AnimationId id, uint32_t now, bool holdExpression = false) override;
 void playTouchWake(RobotState source,uint32_t now) override;
 void playTouchLookUp(RobotState source, uint32_t now) override;
 void playBase(RobotState state, uint32_t now) override;
 void playShakeRecovery(uint32_t now) override;
 void playTouchReturn(uint32_t now) override;
 void update(uint32_t now);
 bool isAnimationFinished() const override { return player_.isFinished(); }
 void showErrorFace() override;
 const desktoprobot::Animation* currentAnimation() const { return player_.getCurrentAnimation(); }
 const desktoprobot::Framebuffer& framebuffer() const { return framebuffer_; }
private:
 U8G2_SH1106_128X64_NONAME_F_HW_I2C display_{U8G2_R0,U8X8_PIN_NONE,Pins::SCL,Pins::SDA};
 desktoprobot::Framebuffer framebuffer_;
 AnimationPlayer player_;
 uint32_t lastUpdate_ = 0;
 bool ready_=false, dirty_=false, error_=false, completionLogged_=false;
 const desktoprobot::Animation* wifiOverlay_=nullptr;
 uint32_t wifiOverlayStart_=0;
 std::size_t wifiOverlayFrame_=static_cast<std::size_t>(-1);
 Log log_=nullptr;
 void* logContext_=nullptr;
 void report(const char* message);
 void render();
 void play(const desktoprobot::Animation& animation, uint32_t now, std::size_t limit = 0);
};
