#include "SleepToTiredAnimation.h"
namespace desktoprobot {
void SleepToTiredAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("SLEEP TO TIRED", {
                eyesClosed, almostClosed, eyesClosed, halfLid, tired
            }, { 500, 500, 280, 650, 500 }, 5);
}
}
