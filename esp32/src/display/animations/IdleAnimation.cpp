#include "IdleAnimation.h"
namespace desktoprobot { void IdleAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
createAnimation(storage,"IDLE",{neutral,shifted(neutral,0,-1),neutral},{1200,1200,500},3,true);
} }
