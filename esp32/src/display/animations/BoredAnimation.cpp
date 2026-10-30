#include "BoredAnimation.h"
namespace desktoprobot {
void BoredAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
            Pose boredLeft = bored;
            boredLeft.gazeX = -3;
            Pose boredBlink = boredLeft;
            boredBlink.eyeHeight = 7;
            boredBlink.rightEyeHeightOffset = 0;

auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("BORED", {
                neutral, bored, shifted(bored, 0, 1), boredLeft,
                shifted(boredBlink, 0, 1), boredLeft, neutral
            }, { 400, 800, 700, 800, 600, 800, 320 }, 7);
}
}
