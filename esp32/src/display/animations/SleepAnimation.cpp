#include "SleepAnimation.h"
namespace desktoprobot { void SleepAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
createAnimation(storage,"SLEEP",{eyesClosed,shifted(eyesClosed,0,-1),eyesClosed},{1200,1200,500},3,true);
} }
