#include "LookLeftAnimation.h"
namespace desktoprobot {
void LookLeftAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("LOOK LEFT", { neutral, glanceLeft, shifted(glanceLeft, -2, 1), glanceLeft, neutral }, { 240, 420, 300, 220, 240 }, 5);
}
}
