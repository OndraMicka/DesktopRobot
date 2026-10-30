#include "LookRightAnimation.h"
namespace desktoprobot {
void LookRightAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("LOOK RIGHT", { neutral, glanceRight, shifted(glanceRight, 2, 1), glanceRight, neutral }, { 240, 420, 300, 220, 240 }, 5);
}
}
