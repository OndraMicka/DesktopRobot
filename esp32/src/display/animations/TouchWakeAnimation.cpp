#include "TouchWakeAnimation.h"
#include "TouchAnimationBuilder.h"
namespace desktoprobot {
const Animation& TouchWakeAnimation::get(RobotState source) {
 using namespace animationdata;
 return buildTouchPose(source==RobotState::Sleep ? eyesClosed : tired,neutral,220,"TOUCH WAKE");
}
}
