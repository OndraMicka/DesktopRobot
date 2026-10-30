#include "ShakingAnimation.h"
namespace desktoprobot {
void ShakingAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
            Pose braced = angry; braced.eyeHeight=11; braced.eyeTilt=3;
auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("SHAKING", {
                shifted(braced, -3, 1), shifted(braced, 3, -1),
                shifted(braced, -2, -1), shifted(braced, 2, 1), shifted(braced, -3, 1)
            }, { 80, 80, 80, 80, 1 }, 5);
storage.animation.loop=true;
}
}
