#pragma once
#include "behavior/BehaviourProgram.h"
#include "display/OutputDisplay.h"
#include "diagnostics/HardwareDiagnostics.h"
class Robot {
public:
 bool begin();
 void update();
 void setLocalTime(uint64_t localEpochSeconds);
 void handleEvent(InputEvent event) { behavior_.handleEvent(event); }
 const DiagnosticResult& diagnostics() const { return result_; }
private:
 LocalTime localTime_{};
 uint32_t clockTick_=0, clockRemainder_=0;
 char timeCommand_[40]{};
 unsigned timeCommandLength_=0;
 void updateClock(uint32_t now);
 InputController input_;
 OutputDisplay display_;
 static uint32_t randomValue(void*);
 BehaviourProgram behavior_{display_, {}, randomValue};
 DiagnosticResult result_{false,"Not started"};
};
