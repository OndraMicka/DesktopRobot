#include "IdleYawnAnimation.h"
namespace desktoprobot {
void IdleYawnAnimation::build(animationdata::AnimationStorage& storage) {
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
add("YAWN", {
                neutral, shifted(halfLid, 0, 2), yawnStart, yawnStretch,
                yawnRelease, shifted(tired, 1, 2), neutral
            }, { 260, 420, 420, 740, 420, 360, 220 }, 7);
}
}
