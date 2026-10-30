#pragma once
#include "../AnimationId.h"
const desktoprobot::Animation& getAnimation(AnimationId id);
const desktoprobot::Animation& getErrorAnimation();

const desktoprobot::Animation& getTouchWakeAnimation(desktoprobot::RobotState source, bool lookUp=false);
