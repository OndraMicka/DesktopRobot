#include "IdleToTiredAnimation.h"
namespace desktoprobot {
void IdleToTiredAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
            [[maybe_unused]] const Pose yawnStart = mouthSized(shifted(yawn, -2, 2), 10, 10);
            Pose yawnStretch = mouthSized(shifted(yawn, -3, -4), 18, 22);
            yawnStretch.eyeHeight = 3;
            yawnStretch.eyeWidth = 34;
            [[maybe_unused]] const Pose yawnRelease = mouthSized(shifted(yawn, 2, -1), 12, 14);
auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("IDLE TO TIRED", {
                neutral, halfLid, yawnStart, yawnStretch, yawnRelease, halfLid, tired
            }, { 350, 420, 350, 650, 400, 500, 450 }, 7);
}
}
