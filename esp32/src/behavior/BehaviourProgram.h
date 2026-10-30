#pragma once
#include "../input/InputController.h"
#include "../display/OutputDisplay.h"
enum class RobotState { IDLE, TIRED, SLEEP, ERROR };
class BehaviourProgram {
public:
 explicit BehaviourProgram(OutputDisplay& display): display_(display) {}
 void begin();
 void handleEvent(InputEvent event);
 void update(uint32_t nowMs);
 void setState(RobotState state);
 RobotState state() const { return state_; }
private:
 OutputDisplay& display_;
 RobotState state_=RobotState::IDLE;
 bool transient_=false;
 uint32_t idleSince_=0;
 void playBase();
};
