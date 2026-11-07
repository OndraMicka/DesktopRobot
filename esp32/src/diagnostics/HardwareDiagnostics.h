#pragma once
#include "../display/OutputDisplay.h"
#include "../input/InputController.h"
struct DiagnosticResult {
 bool ok;
 const char* error;
};
class HardwareDiagnostics {
public:
 using Log = void (*)(void*, const char*);
 DiagnosticResult run(OutputDisplay& display, InputController& input, Log log = nullptr, void* context = nullptr);
};
