#include "ListeningAnimation.h"
namespace desktoprobot {
void ListeningAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("LISTENING", { neutral, glanceRight, shifted(glanceRight, 0, -2), glanceRight, neutral }, { 260, 500, 400, 500, 260 }, 5);
}
}
