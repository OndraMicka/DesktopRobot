#include "Robot.h"
#include <Arduino.h>
#include <cstdlib>
#include <cstring>
#ifdef ARDUINO
#include <esp_system.h>
#endif
bool Robot::begin() {
 HardwareDiagnostics diagnostics;
 result_=diagnostics.run(display_,input_);
 if (!result_.ok) {
  Serial.printf("[ERROR] %s\n",result_.error);
  behavior_.setState(RobotState::ERROR);
 } else {
  Serial.println("[CLOCK] UNSET: send TIME <local epoch seconds> to enable schedule");
  behavior_.begin(millis());
 }
 return result_.ok;
}
void Robot::update() {
 if (behavior_.state()==RobotState::ERROR) { display_.showErrorFace(); return; }
 const uint32_t now=millis();
 updateClock(now);
 behavior_.update(now,localTime_);
 input_.update(now);
 InputEvent event;
 while (input_.pollEvent(event)) behavior_.handleEvent(event);
 display_.update(now);
 behavior_.update(now,localTime_);
}

void Robot::setLocalTime(uint64_t seconds) {
 localTime_={static_cast<int64_t>(seconds/86400),static_cast<uint32_t>(seconds%86400),true};
 clockTick_=millis(); clockRemainder_=0;
 Serial.println("[CLOCK] Local time configured");
}
void Robot::updateClock(uint32_t now) {
#ifdef ARDUINO
 while (Serial.available()) {
  const char c=static_cast<char>(Serial.read());
  if (c=='\r') continue;
  if (c=='\n') {
   timeCommand_[timeCommandLength_]='\0';
   if (std::strncmp(timeCommand_,"TIME ",5)==0) {
    char* end=nullptr;
    const auto seconds=std::strtoull(timeCommand_+5,&end,10);
    if (end!=timeCommand_+5 && *end=='\0' && timeCommand_[5]!='-') { setLocalTime(seconds); clockTick_=now; }
    else Serial.println("[CLOCK] Invalid TIME command");
   }
   timeCommandLength_=0;
  } else if (timeCommandLength_+1<sizeof(timeCommand_)) timeCommand_[timeCommandLength_++]=c;
  else timeCommandLength_=0;
 }
#endif
 if (!localTime_.valid) return;
 const uint64_t elapsed=uint64_t(now-clockTick_)+clockRemainder_;
 clockTick_=now; clockRemainder_=elapsed%1000;
 const uint64_t seconds=localTime_.second+elapsed/1000;
 localTime_.day+=seconds/86400; localTime_.second=seconds%86400;
}

uint32_t Robot::randomValue(void*) {
#ifdef ARDUINO
 return esp_random();
#else
 return 1234567;
#endif
}
