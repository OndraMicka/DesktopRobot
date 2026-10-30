#include "TouchLookUpAnimation.h"
#include "TouchAnimationBuilder.h"
namespace desktoprobot { const Animation& TouchLookUpAnimation::get() {
 return buildTouchPose(animationdata::neutral,animationdata::glanceUp,200,"TOUCH LOOK UP");
} }
