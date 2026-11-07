#pragma once
#include "behavior/BehaviourProgram.h"
#include "display/OutputDisplay.h"
#include "diagnostics/HardwareDiagnostics.h"
#include "config/Debug.h"
#include "time/TimeManager.h"
#include "sound/SoundManager.h"
#if ROBOT_DEBUG_CONSOLE
#include "debug/DebugConsole.h"
#endif
class Robot {
public:
 bool begin();
 void update();
 void setLocalTime(uint64_t localEpochSeconds);
 void handleEvent(InputEvent event);
 void requestState(RobotState state, bool morning = false);
 void simulateShake(uint32_t durationMs);
 void setSimulatedTime(uint32_t second);
 void useAutoTime();
 void describeStatus(char* buffer, std::size_t size) const;
 void describeComponents(char* buffer, std::size_t size) const;
 void formatLogLine(const char* message, char* buffer, std::size_t size) const;
 RobotState state() const { return behavior_.state(); }
 const DiagnosticResult& diagnostics() const { return result_; }
 bool soundAvailable() const { return sound_.healthy(); }
 bool motionAvailable() const { return input_.motionAvailable(); }
 bool displayAvailable() const { return display_.ready(); }
private:
 LocalTime realTime_{}, simulatedTime_{};
 bool simulated_=false, shakePending_=false, ntpAnimationShown_=false, ntpFailureAnimationShown_=false;
 bool starting_=false;
 uint32_t clockTick_=0, simulatedRemainder_=0;
 uint32_t shakeStart_=0, shakeDuration_=0;
 RobotState loggedState_=RobotState::IDLE;
 bool stateLogged_=false;
 void updateClock(uint32_t now);
 LocalTime effectiveTime() const { return simulated_ ? simulatedTime_ : realTime_; }
 void reportState();
 void log(const char* message);
 static void outputLog(void* context, const char* message);
 TimeManager timeManager_;
 InputController input_;
 OutputDisplay display_;
 desktoprobot::SoundManager sound_;
 bool soundFaultLogged_ = false;
 static uint32_t randomValue(void*);
 BehaviourProgram behavior_{display_, sound_, {}, randomValue};
 DiagnosticResult result_{false,"Not started"};
#if ROBOT_DEBUG_CONSOLE
 DebugConsole console_{*this};
#endif
};
