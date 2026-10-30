#include "SlowEyeMovementAnimation.h"
namespace desktoprobot {
void SlowEyeMovementAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
auto add = [&](const char* label, const std::array<Pose,7>& poses, std::array<uint32_t,7> durations, std::size_t count=3) {
for(std::size_t i=0;i<count;++i) durations[i]+=durations[i]/2;
createAnimation(storage,label,poses,durations,count);
};
add("SLOW EYE MOVEMENT", {
                tired,
                Pose{ 0, 0, 32, 12, -7, 0, 0, 0, Mouth::None, NO_Z },
                tired,
                Pose{ 0, 0, 32, 12, 7, 0, 0, 0, Mouth::None, NO_Z },
                tired
            }, { 360, 700, 500, 700, 360 }, 5);
}
}
