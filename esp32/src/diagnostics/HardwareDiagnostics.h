#pragma once
#include "../display/OutputDisplay.h"
#include "../input/InputController.h"
struct DiagnosticResult {
 bool ok;
 const char* error;
};
class HardwareDiagnostics {
public:
 DiagnosticResult run(OutputDisplay& display, InputController& input);
};
