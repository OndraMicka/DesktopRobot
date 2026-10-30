#include "TiredToSleepAnimation.h"
namespace desktoprobot {
void TiredToSleepAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("TIRED TO SLEEP", {
                tired, shifted(almostClosed, 0, 1), tired,
                shifted(almostClosed, 0, 1), eyesClosed
            }, { 420, 560, 340, 680, 600 }, 5);
}
}
