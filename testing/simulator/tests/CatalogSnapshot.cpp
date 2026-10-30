#include "animation/TestAnimations.h"
#include <cstdint>
#include <iostream>
using namespace desktoprobot;
uint64_t hash = 1469598103934665603ULL;
void byte(uint8_t value) { hash = (hash ^ value)*1099511628211ULL; }
void snapshot(const Animation& animation) {
    byte(static_cast<uint8_t>(animation.frameCount)); byte(animation.loop);
    for (std::size_t i=0; i<animation.frameCount; ++i) {
        const auto& f=animation.frames[i];
        for (unsigned j=0;j<4;++j) byte(static_cast<uint8_t>(f.durationMs>>(8*j)));
        Framebuffer fb; drawFrame(f,fb);
        for(auto pixel:fb.data()) byte(pixel);
    }
}
int main() {
    for (const auto& d:getAnimationTestDefinitions()) snapshot(getTestAnimation(d.id));
    for(auto state:{RobotState::Idle,RobotState::Tired,RobotState::Sleep}) snapshot(getStateAnimation(state));
    const auto count=getTestAnimation(TestAnimationId::Shaking).frameCount;
    for(std::size_t i=0;i<count;++i) {
        const auto& shake=getTestAnimation(TestAnimationId::Shaking);
        snapshot(getShakeRecoveryAnimation(shake.frames[i]));
    }
    std::cout<<hash<<'\n';
}
