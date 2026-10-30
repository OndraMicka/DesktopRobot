#include "TiredToIdleAnimation.h"
namespace desktoprobot {
void TiredToIdleAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("TIRED TO IDLE", {
                tired, eyesClosed, halfLid, neutral, glanceLeft, glanceRight, neutral
            }, { 320, 180, 450, 350, 320, 320, 300 }, 7);
}
}
