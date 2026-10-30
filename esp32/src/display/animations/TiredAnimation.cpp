#include "TiredAnimation.h"
namespace desktoprobot { void TiredAnimation::build(animationdata::AnimationStorage& storage) {
using namespace animationdata;
createAnimation(storage,"TIRED",{tired,shifted(tired,0,-1),tired},{1200,1200,500},3,true);
} }
