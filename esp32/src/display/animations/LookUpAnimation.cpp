#include "LookUpAnimation.h"
namespace desktoprobot {
void LookUpAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("LOOK UP", { neutral, glanceUp, shifted(glanceUp, 0, 2), glanceUp, neutral }, { 240, 440, 260, 220, 260 }, 5);
}
}
