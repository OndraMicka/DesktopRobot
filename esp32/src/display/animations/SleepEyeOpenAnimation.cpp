#include "SleepEyeOpenAnimation.h"
namespace desktoprobot {
void SleepEyeOpenAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
            Pose sleepPeek=eyesClosed; sleepPeek.eyeHeight=14; sleepPeek.rightEyeHeightOffset=-11; sleepPeek.gazeX=-3;
auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("SLEEP EYE OPEN", {
                eyesClosed, sleepPeek, shifted(sleepPeek, 0, -1), sleepPeek, eyesClosed
            }, { 360, 580, 480, 400, 360 }, 5);
}
}
