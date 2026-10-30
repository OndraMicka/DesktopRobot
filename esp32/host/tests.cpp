#include <Arduino.h>
#include <Wire.h>
#include <cassert>
#include <iostream>
#include "Robot.h"
#include "display/animations/AnimationCatalog.h"

void reset() {
 testNow=0; testPin=0; testReads=0; Serial.log.clear();
 Wire.busOK=true; Wire.ack=0; displayOK=true; sentFrames=0; errorTextDrawn=false;
}
void tick(InputController& input,uint32_t t,int pin) { testNow=t; testPin=pin; input.update(t); }
void expect(InputController& input,InputEvent expected) {
 InputEvent event;
 assert(input.pollEvent(event)==(expected!=InputEvent::NONE)); assert(event==expected);
 assert(!input.pollEvent(event));
}
int main() {
 reset(); InputController input; input.begin();
 tick(input,10,1); tick(input,25,0); tick(input,70,0); expect(input,InputEvent::NONE);
 tick(input,100,1); tick(input,135,1); expect(input,InputEvent::NONE);
 tick(input,200,0); tick(input,235,0); expect(input,InputEvent::TAP);
 tick(input,300,1); tick(input,335,1); tick(input,1299,1); expect(input,InputEvent::NONE);
 tick(input,1300,1); expect(input,InputEvent::HOLD);
 tick(input,2500,1); expect(input,InputEvent::NONE);
 tick(input,2600,0); tick(input,2635,0); expect(input,InputEvent::NONE);
 // A release just below the threshold remains TAP despite debounce latency.
 tick(input,3000,1); tick(input,3035,1); tick(input,3999,0); tick(input,4034,0); expect(input,InputEvent::TAP);
 // A slow loop that missed the hold threshold still classifies a long release correctly.
 tick(input,5000,1); tick(input,5035,1); tick(input,6100,0); tick(input,6135,0); expect(input,InputEvent::HOLD);
 // millis wraparound, including a sensor already held at boot.
 testNow=UINT32_MAX-500; testPin=1; input.begin();
 tick(input,499,1); expect(input,InputEvent::HOLD);

 AnimationPlayer player;
 player.play(getAnimation(AnimationId::BLINK),UINT32_MAX-99);
 assert(!player.update(UINT32_MAX-40)); assert(player.update(UINT32_MAX-39));
 player.update(140); assert(player.isFinished());
 assert(&player.frame()==&getAnimation(AnimationId::BLINK).frames[2]);
 assert(!player.update(10000)); // Completion must latch; no automatic IDLE.
 player.play(getAnimation(AnimationId::SMILE),0); assert(!player.isFinished());
 player.play(getAnimation(AnimationId::IDLE),0); player.update(1000000000); assert(!player.isFinished());

 reset(); OutputDisplay display; assert(display.begin()); BehaviourProgram behavior(display);
 behavior.begin(); display.update(0); assert(sentFrames==1);
 behavior.update(3999); assert(Serial.log.find("[ANIM] BLINK")==std::string::npos);
 testNow=4000; behavior.update(testNow); assert(Serial.log.find("[ANIM] BLINK")!=std::string::npos);
 behavior.handleEvent(InputEvent::TAP); assert(Serial.log.find("[ANIM] ANGRY")!=std::string::npos);
 testNow=4100; behavior.handleEvent(InputEvent::HOLD); display.update(testNow);
 assert(Serial.log.find("[ANIM] SMILE")!=std::string::npos);
 testNow=5550; display.update(testNow); assert(display.isAnimationFinished());
 behavior.update(testNow); assert(!display.isAnimationFinished());
 const auto log=Serial.log;
 display.update(testNow); behavior.update(testNow); assert(Serial.log==log);
 behavior.setState(RobotState::TIRED); behavior.update(100000); assert(behavior.state()==RobotState::TIRED);
 behavior.setState(RobotState::SLEEP); assert(behavior.state()==RobotState::SLEEP);
 behavior.setState(RobotState::ERROR); assert(errorTextDrawn);
 const int frames=sentFrames; behavior.handleEvent(InputEvent::TAP); behavior.setState(RobotState::IDLE);
 display.playAnimation(AnimationId::SMILE); display.update(999999); assert(sentFrames==frames);
 assert(behavior.state()==RobotState::ERROR);

 reset(); Wire.ack=2; Robot failed; assert(!failed.begin()); assert(!failed.diagnostics().ok);
 assert(Serial.log.find("[STATE] ERROR")!=std::string::npos);
 const int reads=testReads;
 for (testNow=0;testNow<10000;++testNow) { testPin=1; failed.update(); }
 assert(testReads==reads); assert(sentFrames==0);
 assert(Serial.log.find("[ANIM]")==std::string::npos);
 reset(); Wire.busOK=false; Robot badBus; assert(!badBus.begin());
 reset(); displayOK=false; Robot badInit; assert(!badInit.begin());
 reset(); Robot good; assert(good.begin()); good.update(); assert(sentFrames==1);
 assert(Serial.log.find("CONFIGURED (not electrically verifiable)")!=std::string::npos);
 std::cout<<"PASS: touch, debounce, hold, rollover, animation completion/interruption, states, startup and ERROR\n";
}
