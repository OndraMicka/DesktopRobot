#include <Arduino.h>
#include <Wire.h>
#include <cassert>
#include <iostream>
#include <vector>
#include "Robot.h"
#include "display/animations/AnimationCatalog.h"

struct FakeOutput : AnimationOutput {
    std::vector<AnimationId> history;
    RobotState baseState = RobotState::IDLE;
    bool done = false, error = false, hold = false;
    unsigned bases = 0, recoveries = 0;
    void playAnimation(AnimationId id, uint32_t, bool held = false) override { history.push_back(id); done=false; hold=held; }
    void playBase(RobotState s, uint32_t) override { baseState=s; ++bases; done=false; }
    void playShakeRecovery(uint32_t) override { ++recoveries; done=false; }
    bool isAnimationFinished() const override { return done; }
    void showErrorFace() override { error=true; }
    AnimationId last() const { assert(!history.empty()); return history.back(); }
};
LocalTime clockAt(unsigned h,unsigned m=0,unsigned sec=0,int64_t day=10) { return {day,h*3600+m*60+sec,true}; }
void complete(BehaviourProgram& b, FakeOutput& o, uint32_t& now, LocalTime time={}) {
    for (unsigned i=0;i<8 && b.busy();++i) { o.done=true; b.update(++now,time); }
    assert(!b.busy());
}
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
    tick(input,100,1); tick(input,135,1); tick(input,200,0); tick(input,235,0); expect(input,InputEvent::TAP);
    tick(input,300,1); tick(input,335,1); tick(input,999,1); expect(input,InputEvent::NONE);
    tick(input,1000,1); expect(input,InputEvent::HOLD);
    tick(input,2500,1); expect(input,InputEvent::NONE);
    tick(input,2600,0); tick(input,2635,0); expect(input,InputEvent::RELEASE);
    tick(input,3000,1); tick(input,3035,1); tick(input,3699,0); tick(input,3734,0); expect(input,InputEvent::TAP);
    tick(input,5000,1); tick(input,5035,1); tick(input,6100,0); tick(input,6135,0); expect(input,InputEvent::HOLD);
    // Even a threshold missed by a slow loop must provide HOLD then RELEASE.
    tick(input,6200,0); expect(input,InputEvent::RELEASE);
    testNow=UINT32_MAX-500; testPin=1; input.begin(); tick(input,199,1); expect(input,InputEvent::HOLD);

    // All six transitions select exact catalog identifiers and preserve destinations.
    for (auto from : {RobotState::IDLE,RobotState::TIRED,RobotState::SLEEP}) {
        for (auto to : {RobotState::IDLE,RobotState::TIRED,RobotState::SLEEP}) {
            FakeOutput o; BehaviourProgram b(o); b.enableSpontaneous(false); uint32_t now=0; b.begin();
            b.setState(from); complete(b,o,now);
            auto size=o.history.size(); b.setState(to);
            if (from==to) { assert(o.history.size()==size); continue; }
            const auto* def=desktoprobot::getStateTransition(static_cast<desktoprobot::RobotState>(from),static_cast<desktoprobot::RobotState>(to));
            assert(def && o.last()==def->id); assert(b.state()==to);
            complete(b,o,now); assert(o.baseState==to);
        }
    }
    // Exact boundaries, sequence order, midnight and clock jumps.
    FakeOutput o; BehaviourProgram b(o); b.enableSpontaneous(false); uint32_t now=0;
    b.begin(now,clockAt(20,59,59));
    b.update(++now,clockAt(21)); assert(b.state()==RobotState::TIRED && o.last()==AnimationId::IdleToTired);
    complete(b,o,now,clockAt(21));
    b.update(++now,clockAt(21,30)); assert(b.state()==RobotState::SLEEP && o.last()==AnimationId::TiredToSleep);
    complete(b,o,now,clockAt(21,30));
    b.update(++now,clockAt(0,0,0,11)); assert(!b.busy());
    b.update(++now,clockAt(7,0,0,11)); assert(o.last()==AnimationId::WakeUp);
    o.done=true; b.update(++now,clockAt(7,0,0,11)); assert(o.last()==AnimationId::IdleYawn);
    o.done=true; b.update(++now,clockAt(7,0,0,11)); assert(o.last()==AnimationId::LookAround);
    complete(b,o,now,clockAt(7,0,0,11)); assert(b.state()==RobotState::IDLE);
    b.update(++now,clockAt(22,0,0,12)); assert(b.state()==RobotState::SLEEP && o.last()==AnimationId::IdleToSleep);
    complete(b,o,now,clockAt(22,0,0,12));

    for (auto state : {RobotState::IDLE,RobotState::TIRED,RobotState::SLEEP}) {
        FakeOutput out; BehaviourProgram brain(out); brain.enableSpontaneous(false); uint32_t t=0; brain.begin();
        brain.setState(state); complete(brain,out,t);
        brain.handleEvent(InputEvent::TAP);
        if(state==RobotState::IDLE) assert(out.last()==AnimationId::Angry);
        if(state==RobotState::TIRED) assert(out.last()==AnimationId::TiredToIdle);
        if(state==RobotState::SLEEP) assert(out.last()==AnimationId::SleepToTired);
        complete(brain,out,t);
        assert(brain.state()==(state==RobotState::SLEEP ? RobotState::TIRED : RobotState::IDLE));
    }
    for (auto state : {RobotState::IDLE,RobotState::TIRED,RobotState::SLEEP}) {
        FakeOutput out; BehaviourProgram brain(out); brain.enableSpontaneous(false); uint32_t t=0; brain.begin();
        brain.setState(state); complete(brain,out,t); brain.handleEvent(InputEvent::HOLD);
        for(unsigned i=0;i<5;++i) { out.done=true; brain.update(++t); }
        assert(out.last()==AnimationId::Smile && out.hold && brain.busy());
        auto count=out.history.size(); out.done=true; brain.update(t+100000); assert(out.history.size()==count);
        brain.handleEvent(InputEvent::RELEASE); assert(!brain.busy());
        assert(brain.state()==(state==RobotState::SLEEP ? RobotState::TIRED : state));
    }
    // Release during wake-up finishes the reaction once, without restarting the wake animation.
    { FakeOutput out; BehaviourProgram brain(out); uint32_t t=0; brain.begin(); brain.setState(RobotState::SLEEP); complete(brain,out,t);
      out.history.clear(); brain.handleEvent(InputEvent::HOLD); brain.handleEvent(InputEvent::RELEASE); complete(brain,out,t);
      assert(out.history.size()==3 && out.history[0]==AnimationId::SleepToTired); }
    // A second interaction during opening changes the tail without restarting wake-up.
    { FakeOutput out; BehaviourProgram brain(out); uint32_t t=0; brain.begin(); brain.setState(RobotState::SLEEP); complete(brain,out,t);
      out.history.clear(); brain.handleEvent(InputEvent::TAP); brain.handleEvent(InputEvent::HOLD);
      assert(out.history.size()==1 && out.last()==AnimationId::SleepToTired);
      brain.handleEvent(InputEvent::RELEASE); complete(brain,out,t);
      assert(out.history.size()==3 && out.history[1]==AnimationId::LookUp && out.history[2]==AnimationId::Smile); }
    // Both touch events replace spontaneous playback immediately.
    { FakeOutput out; BehaviourProgram brain(out); brain.begin(); brain.preview(AnimationId::Thinking);
      brain.handleEvent(InputEvent::TAP); assert(out.last()==AnimationId::Angry);
      brain.preview(AnimationId::LookAround); brain.handleEvent(InputEvent::HOLD); assert(out.last()==AnimationId::LookUp); }
    // Early waking cannot expire back to sleep before 07:00.
    { FakeOutput out; BehaviourProgram brain(out); brain.enableSpontaneous(false); uint32_t t=0;
      brain.begin(t,clockAt(6)); complete(brain,out,t,clockAt(6)); brain.handleEvent(InputEvent::TAP); complete(brain,out,t,clockAt(6));
      assert(brain.state()==RobotState::TIRED); brain.update(t+600000,clockAt(6,10)); assert(brain.state()==RobotState::TIRED);
      brain.update(t+3600000,clockAt(7)); assert(brain.state()==RobotState::IDLE && out.last()==AnimationId::TiredToIdle); }
    // Night waking expires after five minutes; TIRED tap expires in two stages.
    { FakeOutput out; BehaviourProgram brain(out); brain.enableSpontaneous(false); uint32_t t=0;
      brain.begin(t,clockAt(22)); complete(brain,out,t,clockAt(22)); brain.handleEvent(InputEvent::TAP); complete(brain,out,t,clockAt(22));
      brain.update(t+300000,clockAt(22,5)); assert(brain.state()==RobotState::SLEEP); }
    { FakeOutput out; BehaviourProgram brain(out); brain.enableSpontaneous(false); uint32_t t=0;
      brain.begin(t,clockAt(21,5)); complete(brain,out,t,clockAt(21,5)); brain.handleEvent(InputEvent::TAP); complete(brain,out,t,clockAt(21,5));
      brain.update(t+300000,clockAt(21,10)); assert(brain.state()==RobotState::TIRED);
      t+=300000; complete(brain,out,t,clockAt(21,10)); brain.update(t+300000,clockAt(21,15)); assert(brain.state()==RobotState::SLEEP); }
    // Schedule overrides a held interaction; subsequent release cannot restore its old state.
    { FakeOutput out; BehaviourProgram brain(out); uint32_t t=0; brain.begin(t,clockAt(20,59)); brain.handleEvent(InputEvent::HOLD);
      brain.update(++t,clockAt(21)); assert(out.last()==AnimationId::IdleToTired);
      auto size=out.history.size(); brain.handleEvent(InputEvent::TAP); assert(out.history.size()==size);
      brain.handleEvent(InputEvent::RELEASE); complete(brain,out,t,clockAt(21)); assert(brain.state()==RobotState::TIRED); }
    // Shake lifecycle, recovery interruption, sleep state preservation and ERROR latch.
    { FakeOutput out; BehaviourProgram brain(out); uint32_t t=0; brain.begin(); brain.setState(RobotState::SLEEP); complete(brain,out,t);
      brain.handleEvent(InputEvent::SHAKE_START); assert(out.last()==AnimationId::Shaking);
      brain.handleEvent(InputEvent::SHAKE_END); assert(out.recoveries==1);
      brain.handleEvent(InputEvent::SHAKE_START); assert(out.last()==AnimationId::Shaking);
      brain.handleEvent(InputEvent::SHAKE_END); complete(brain,out,t); assert(brain.state()==RobotState::SLEEP);
      brain.setState(RobotState::ERROR); auto count=out.history.size(); brain.handleEvent(InputEvent::HOLD); brain.handleEvent(InputEvent::SHAKE_START);
      brain.setState(RobotState::IDLE); brain.update(999999,clockAt(7)); assert(out.error && brain.state()==RobotState::ERROR && out.history.size()==count); }
    // Deterministic activities, independent blink timer, no repeats and no interference with HOLD.
    { FakeOutput out; BehaviourConfig c; c.randomSeed=19; c.activityMinMs[0]=c.activityMaxMs[0]=10000;
      c.blinkMinMs[0]=c.blinkMaxMs[0]=1000; c.quietWeight=0;
      BehaviourProgram brain(out,c); uint32_t t=0; brain.begin();
      for(t=1000;t<10000;t+=1000) { brain.update(t); out.done=true; brain.update(t+1); }
      brain.update(10000); assert(out.last()!=AnimationId::Blink); const auto first=out.last();
      complete(brain,out,t); brain.handleEvent(InputEvent::HOLD);
      for(unsigned i=0;i<3;++i) { out.done=true; brain.update(t+=100000); }
      assert(out.last()==AnimationId::Smile); brain.handleEvent(InputEvent::RELEASE);
      brain.update(t+10000); assert(out.last()!=first); }
    // Cooldowns exclude otherwise eligible activities, and quiet decisions preserve base.
    { FakeOutput out; BehaviourConfig c;
      for(auto& a:c.activities) a.weight=0;
      c.activities[0].weight=c.activities[1].weight=1;
      c.activities[0].cooldownMs=c.activities[1].cooldownMs=100000;
      c.activityMinMs[0]=c.activityMaxMs[0]=10000; c.blinkMinMs[0]=c.blinkMaxMs[0]=1000000; c.quietWeight=0;
      BehaviourProgram brain(out,c,[](void*)->uint32_t { return 0; }); uint32_t t=0; brain.begin();
      brain.update(t=10000); assert(out.last()==AnimationId::LookAround); complete(brain,out,t);
      brain.update(t+=10000); assert(out.last()==AnimationId::Thinking); complete(brain,out,t);
      auto size=out.history.size(); brain.update(t+10000); assert(out.history.size()==size && !brain.busy()); }
    // Activity selection respects state; SLEEP never emits daytime activities or blinks.
    for(auto state:{RobotState::TIRED,RobotState::SLEEP}) {
      FakeOutput out; BehaviourConfig c; auto idx=static_cast<unsigned>(state);
      c.activityMinMs[idx]=c.activityMaxMs[idx]=1000; c.quietWeight=0;
      BehaviourProgram brain(out,c); uint32_t t=0; brain.begin(); brain.setState(state); complete(brain,out,t);
      out.history.clear();
      for(unsigned i=0;i<30;++i) {
        brain.update(t+=5000); complete(brain,out,t);
      }
      assert(!out.history.empty());
      for(auto id:out.history) {
        if(state==RobotState::SLEEP) assert(id==AnimationId::SleepZ || id==AnimationId::SleepEyeOpen);
        else assert(id==AnimationId::TiredYawn || id==AnimationId::SlowEyeMovement || id==AnimationId::SlowBlink);
      }
    }
    // Scheduler elapsed time and animation playback are safe across millis wraparound.
    { FakeOutput out; BehaviourConfig c; c.activityMinMs[0]=c.activityMaxMs[0]=1000;
      c.blinkMinMs[0]=c.blinkMaxMs[0]=100000; c.quietWeight=0;
      BehaviourProgram brain(out,c); brain.begin(UINT32_MAX-500); brain.update(499); assert(brain.busy()); }
    { AnimationPlayer p; const auto& animation=getAnimation(AnimationId::Blink); uint32_t duration=0;
      for(std::size_t i=0;i<animation.frameCount;++i) duration+=animation.frames[i].durationMs;
      p.play(animation); p.update(duration-1); assert(!p.isFinished()); p.update(1); assert(p.isFinished());
      const auto* last=&p.frame(); p.update(UINT32_MAX); assert(&p.frame()==last);
      p.play(desktoprobot::getStateAnimation(desktoprobot::RobotState::Idle)); p.update(UINT32_MAX); assert(!p.isFinished()); }
    // Hardware output holds a real smiling frame and recovery starts with identical pixels.
    reset(); OutputDisplay display; assert(display.begin());
    display.playAnimation(AnimationId::Smile,0,true); display.update(10000); assert(display.isAnimationFinished());
    auto smile=display.framebuffer().data(); display.update(20000); assert(display.framebuffer().data()==smile);
    display.playAnimation(AnimationId::Shaking,20000); display.update(20150); auto before=display.framebuffer().data();
    display.playShakeRecovery(20150); display.update(20150); assert(display.framebuffer().data()==before);
    display.showErrorFace(); assert(errorTextDrawn);
    reset(); Wire.ack=2; Robot failed; assert(!failed.begin()); const int reads=testReads;
    for(testNow=0;testNow<10000;++testNow) { failed.update(); }
    assert(testReads==reads && sentFrames==0);
    reset(); Wire.busOK=false; Robot badBus; assert(!badBus.begin());
    reset(); displayOK=false; Robot badInit; assert(!badInit.begin());
    reset(); Robot good; assert(good.begin()); good.setLocalTime(21*3600); good.update();
    assert(Serial.log.find("CONFIGURED (not electrically verifiable)")!=std::string::npos);
    std::cout<<"PASS: schedule, six transitions, touch/release, temporary wakes, priorities, shake/recovery, activities, hardware diagnostics\n";
}
