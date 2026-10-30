#include "SmileAnimation.h"
namespace desktoprobot {
void SmileAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("SMILE", {
                neutral, happy, shifted(happy, 0, -1), happy, neutral
            }, { 320, 750, 650, 900, 320 }, 5);
}
}
