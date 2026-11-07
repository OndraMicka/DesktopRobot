#include "Robot.h"
#include <Arduino.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef ARDUINO
#include <esp_system.h>

#endif
namespace {
const char* stateName(RobotState state) {
 const char* names[]={"IDLE","TIRED","SLEEP","ERROR"};
 return names[static_cast<unsigned>(state)];
}
void advance(LocalTime& time, uint32_t& remainder, uint32_t elapsed) {
 if (!time.valid) return;
 const uint64_t ms=uint64_t(remainder)+elapsed;
 remainder=ms%1000;
 const uint64_t seconds=time.second+ms/1000;
 time.day+=seconds/86400; time.second=seconds%86400;
}
}
void Robot::log(const char* message) {
#if ROBOT_DEBUG_CONSOLE
 console_.log("%s\n",message);
#else
 Serial.println(message);
#endif
}
void Robot::outputLog(void* context, const char* message) { static_cast<Robot*>(context)->log(message); }
void Robot::reportState() {
 if (!stateLogged_ || loggedState_!=state()) {
  loggedState_=state(); stateLogged_=true;
  char message[48]; std::snprintf(message,sizeof(message),"[STATE] %s",stateName(state())); log(message);
 }
}
bool Robot::begin() {
 clockTick_=millis();
 display_.setLog(outputLog,this);
 HardwareDiagnostics diagnostics;
 result_=diagnostics.run(display_,input_);
 if (!sound_.begin()) {
  log("[SOUND] Initialization failed; continuing without sound");
  soundFaultLogged_=true;
 } else log("[SOUND] GPIO3 LEDC ready; output silent (electrical load not verified)");
 if (!result_.ok) {
  char message[160]; std::snprintf(message,sizeof(message),"[ERROR] %s",result_.error); log(message);
  behavior_.setState(RobotState::ERROR);
 } else {
  timeManager_.begin(millis(),outputLog,this);
  updateClock(millis());
  log(realTime_.valid ? "[CLOCK] AUTO ready" : "[CLOCK] AUTO UNSET: waiting for verified NTP or manual TIME");
  starting_=true;
  display_.playPowerOn(millis());
 }
 reportState();
#if ROBOT_DEBUG_CONSOLE
 console_.log("[CONSOLE] Ready at 115200 baud. Type help\n");
#endif
 return result_.ok;
}
void Robot::update() {
 const uint32_t now=millis();
 sound_.update(now); // Host/manual driver; ESP32 advances in the ESP Timer task.
 if (!sound_.healthy() && !soundFaultLogged_) {
  log("[SOUND] Output failed; sound disabled until restart"); soundFaultLogged_=true;
 }
 if (state()!=RobotState::ERROR) {
  updateClock(now);
 }
#if ROBOT_DEBUG_CONSOLE
 console_.update(); // Read-only status/help/diag remain available in ERROR.
#endif
 if (state()==RobotState::ERROR) { display_.showErrorFace(); return; }
 const uint32_t eventNow=millis();
 if (starting_) {
  // Keep clocks and the console live, but boot owns the display. Discard
  // interactions collected during startup so they cannot replace its frames.
  input_.update(eventNow);
  InputEvent ignored;
  while (input_.pollEvent(ignored)) {}
  display_.update(eventNow);
  if (display_.isAnimationFinished()) {
   starting_=false;
   input_.begin();
   behavior_.begin(eventNow,effectiveTime()); reportState();
  }
  return;
 }
 if (shakePending_ && eventNow-shakeStart_>=shakeDuration_) {
  shakePending_=false; handleEvent(InputEvent::SHAKE_END);
 }
 input_.update(eventNow);
 behavior_.setTouchPending(input_.touchPending());
 // Observe input before allowing automatic playback, while still reconciling
 // scheduled boundaries before delivering the classified interaction.
 behavior_.update(eventNow,effectiveTime()); reportState();
 InputEvent event;
 while (input_.pollEvent(event)) handleEvent(event);
 display_.update(eventNow);
 behavior_.update(eventNow,effectiveTime()); reportState();
}
void Robot::handleEvent(InputEvent event) {
 if (starting_) { log("[BOOT] Input ignored until POWER ON finishes"); return; }
 const char* names[]={"NONE","TAP","HOLD","RELEASE","SHAKE_START","SHAKE_END"};
 char message[112];
 std::snprintf(message,sizeof(message),"[EVENT] %s received state=%s mode=%s%s",names[static_cast<unsigned>(event)],
  stateName(state()),behavior_.modeName(),state()==RobotState::ERROR ? " (blocked by ERROR)" : "");
 log(message);
 behavior_.handleEvent(event); reportState();
}
void Robot::requestState(RobotState target, bool morning) {
 if (starting_) { log("[BOOT] State request ignored until POWER ON finishes"); return; }
 if (state()==RobotState::ERROR) { log("[ERROR] State request blocked: ERROR is latched"); return; }
 const bool scheduled=std::strcmp(behavior_.modeName(),"SCHEDULE")==0;
 behavior_.requestState(target,morning); reportState();
 log(scheduled ? "[REQUEST] Deferred to schedule priority; request ignored" : "[REQUEST] State transition requested");
}
void Robot::simulateShake(uint32_t duration) {
 if (starting_ || state()==RobotState::ERROR || !duration || duration>600000) return;
 shakePending_=true; shakeStart_=millis(); shakeDuration_=duration;
 handleEvent(InputEvent::SHAKE_START);
}
void Robot::setLocalTime(uint64_t seconds) {
 if (state()==RobotState::ERROR) return;
 updateClock(millis());
 timeManager_.setManualLocalTime(seconds,millis());
 realTime_=timeManager_.localTime();
 log("[CLOCK] Real fallback time configured");
 if (!simulated_ && !starting_) behavior_.changeTime(millis(),realTime_);
 reportState();
}
void Robot::setSimulatedTime(uint32_t second) {
 if (state()==RobotState::ERROR || second>=86400) return;
 updateClock(millis());
 const bool enteringSimulation=!simulated_;
 simulatedTime_={simulated_ ? simulatedTime_.day : realTime_.valid ? realTime_.day : 0,second,true};
 simulated_=true; simulatedRemainder_=0;
 if (!starting_) behavior_.changeTime(millis(),simulatedTime_,enteringSimulation);
 reportState();
 log("[CLOCK] SIMULATED time changed; clock continues at normal speed");
}
void Robot::useAutoTime() {
 if (state()==RobotState::ERROR) return;
 updateClock(millis()); simulated_=false; shakePending_=false;
 if (!starting_) behavior_.changeTime(millis(),realTime_,true);
 reportState();
 log(realTime_.valid ? "[CLOCK] AUTO restored; schedule reconciled" : "[CLOCK] AUTO UNSET: real time unavailable; IDLE fallback, schedule inactive");
}
void Robot::updateClock(uint32_t now) {
 const auto elapsed=now-clockTick_; clockTick_=now;
 advance(simulatedTime_,simulatedRemainder_,elapsed);
 const bool wasNtpSynced=timeManager_.status() && std::strcmp(timeManager_.status(),"NTP_SYNCED")==0;
 timeManager_.update(now);
 realTime_=timeManager_.localTime();
 if (!ntpFailureAnimationShown_ && std::strcmp(timeManager_.status(),"RETRY_WAIT")==0) {
  ntpFailureAnimationShown_=true;
  display_.playWifiFailed(now);
 }
 if (!ntpAnimationShown_ && !wasNtpSynced && std::strcmp(timeManager_.status(),"NTP_SYNCED")==0) {
  ntpAnimationShown_=true;
  display_.playWifiConnected(now);
 }
}
void Robot::describeStatus(char* buffer, std::size_t size) const {
 const auto time=effectiveTime();
 const auto* animation=display_.currentAnimation();
 std::snprintf(buffer,size,
  "[STATUS] state=%s animation=%s finished=%s mode=%s\n[CLOCK] mode=%s valid=%s day=%lld time=%02lu:%02lu:%02lu real=%s source=%s wifi=%s\n[REACTION] temporary=%s remaining_ms=%lu held=%s shake_timer=%s\n[ERROR] %s\n[TIME_ERROR] %s",
  stateName(state()),animation ? animation->name : "NONE",display_.isAnimationFinished() ? "yes" : "no",starting_ ? "POWER_ON" : behavior_.modeName(),
  simulated_ ? "SIMULATED" : "AUTO",time.valid ? "yes" : "no",static_cast<long long>(time.day),
  static_cast<unsigned long>(time.second/3600),static_cast<unsigned long>((time.second/60)%60),static_cast<unsigned long>(time.second%60),
  realTime_.valid ? "available" : "UNSET",timeManager_.status(),timeManager_.radioOff() ? "OFF" : "ON",behavior_.temporaryName(),static_cast<unsigned long>(behavior_.temporaryRemainingMs()),
  behavior_.touchHeld() ? "yes" : "no",shakePending_ ? "active" : "off",result_.error ? result_.error : "none",timeManager_.lastError() ? timeManager_.lastError() : "none");
}
uint32_t Robot::randomValue(void*) {
#ifdef ARDUINO
 return esp_random();
#else
 return 1234567;
#endif
}
