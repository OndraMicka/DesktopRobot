#include "Robot.h"
#include <Arduino.h>
bool Robot::begin() {
 HardwareDiagnostics diagnostics;
 result_=diagnostics.run(display_,input_);
 if (!result_.ok) {
  Serial.printf("[ERROR] %s\n",result_.error);
  behavior_.setState(RobotState::ERROR);
 } else behavior_.begin();
 return result_.ok;
}
void Robot::update() {
 if (behavior_.state()==RobotState::ERROR) { display_.showErrorFace(); return; }
 const uint32_t now=millis();
 input_.update(now);
 InputEvent event;
 while (input_.pollEvent(event)) behavior_.handleEvent(event);
 display_.update(now);
 behavior_.update(now);
}
