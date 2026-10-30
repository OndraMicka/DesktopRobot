#pragma once
#include "behavior/BehaviourProgram.h"
#include "diagnostics/HardwareDiagnostics.h"
class Robot {
public:
 bool begin();
 void update();
 const DiagnosticResult& diagnostics() const { return result_; }
private:
 InputController input_;
 OutputDisplay display_;
 BehaviourProgram behavior_{display_};
 DiagnosticResult result_{false,"Not started"};
};
