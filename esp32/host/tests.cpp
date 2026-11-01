#include <Arduino.h>
#include <Wire.h>
#include <cassert>
#include <iostream>
#include <vector>
#include "Robot.h"
#include "TimeManagerTests.h"
#include "SoundManagerTests.h"
#include "display/animations/AnimationCatalog.h"
#include "display/animations/PowerOnAnimation.h"
#include <algorithm>

struct FakeOutput : AnimationOutput {
    std::vector<AnimationId> history;
    RobotState baseState = RobotState::IDLE;
    bool done = false, error = false, hold = false;
    unsigned bases = 0, recoveries = 0;
    void playAnimation(AnimationId id, uint32_t, bool held = false) override { history.push_back(id); done=false; hold=held; }
    unsigned openings=0;
    RobotState gazeSource=RobotState::IDLE;
    void playTouchWake(RobotState,uint32_t) override { ++openings; done=false; }
    void playTouchLookUp(RobotState source,uint32_t now) override { gazeSource=source; playAnimation(AnimationId::LookUp,now); }
    void playBase(RobotState s, uint32_t) override { baseState=s; ++bases; done=false; }
    void playShakeRecovery(uint32_t) override { ++recoveries; done=false; }
    unsigned returns=0;
    void playTouchReturn(uint32_t) override { ++returns; done=false; }
    bool isAnimationFinished() const override { return done; }
    void showErrorFace() override { error=true; }
    AnimationId last() const { assert(!history.empty()); return history.back(); }
};
LocalTime clockAt(unsigned h,unsigned m=0,unsigned sec=0,int64_t day=10) { return {day,h*3600+m*60+sec,true}; }
void complete(BehaviourProgram& b, FakeOutput& o, uint32_t& now, LocalTime time={}) {
    for (unsigned i=0;i<8 && b.busy();++i) { o.done=true; b.update(++now,time); }
    assert(!b.busy());
}
#include "ActivitySelectionTests.h"
#include "BlinkSchedulingTests.h"
void reset() {
    testTouchInterrupt=nullptr; testTouchContext=nullptr;
    testNow=0; testPin=0; testReads=0; Serial.log.clear();
    Serial.rx.clear(); Serial.rxPosition=0; Serial.txCapacity=64;
    Wire.busOK=true; Wire.ack=0; displayOK=true; sentFrames=0; errorTextDrawn=false;
}
void tick(InputController& input,uint32_t t,int pin) { testNow=t; testPin=pin; input.update(t); }
void expect(InputController& input,InputEvent expected) {
    InputEvent event;
    assert(input.pollEvent(event)==(expected!=InputEvent::NONE)); assert(event==expected);
    assert(!input.pollEvent(event));
}
void testConsole();
void testNtpSchedule();
uint32_t powerOnDuration() {
    uint32_t duration=0;
    const auto& animation=desktoprobot::getPowerOnAnimation();
    for(std::size_t i=0;i<animation.frameCount;++i) duration+=animation.frames[i].durationMs;
    return duration;
}
void finishStartup(Robot& robot) { testNow+=powerOnDuration(); robot.update(); }
void testPowerOn() {
    const auto& animation=desktoprobot::getPowerOnAnimation();
    assert(!animation.loop && powerOnDuration()==4350);
    desktoprobot::Framebuffer rendered, expected;
    uint32_t time=0;
    int previousHeight=0;
    for(std::size_t i=0;i<animation.frameCount;++i) {
        const auto& frame=animation.frames[i];
        assert(frame.durationMs>0);
        rendered.clear(); desktoprobot::drawFrame(frame,rendered);
        if(time<900) assert(std::all_of(rendered.data().begin(),rendered.data().end(),[](auto p){return p==0;}));
        if(time<1900) assert(frame.commandCount<=2);
        if(frame.commandCount) {
            assert(frame.commands[0].y2>=previousHeight);
            previousHeight=frame.commands[0].y2;
        }
        if(time>=2300) assert(frame.commandCount==3 && frame.commands[2].x2==20 && frame.commands[2].y2==5);
        time+=frame.durationMs;
    }
    desktoprobot::drawFrame(desktoprobot::getStateAnimation(desktoprobot::RobotState::Idle).frames[0],expected);
    assert(rendered.data()==expected.data());
    for(uint32_t start:{0U,UINT32_MAX-1000U}) {
        reset(); testNow=start;
        Robot robot; assert(robot.begin());
        assert(sentFrames>=1);
        assert(std::all_of(firstDisplayFrame.begin(),firstDisplayFrame.end(),[](auto p){return p==0;}));
        assert(lastDisplayFrame==firstDisplayFrame);
        robot.setLocalTime(12*3600);
        robot.setSimulatedTime(12*3600); robot.useAutoTime();
        robot.requestState(RobotState::SLEEP);
        robot.handleEvent(InputEvent::TAP); robot.simulateShake(100);
        for(uint32_t elapsed=0;elapsed<powerOnDuration();elapsed+=50) {
            testNow=start+elapsed; testPin=(elapsed>=100 && elapsed<300) ? 1 : 0;
            robot.update();
            char status[512]; robot.describeStatus(status,sizeof(status));
            assert(std::string(status).find("animation=POWER ON")!=std::string::npos);
            if(elapsed<900) assert(lastDisplayFrame==firstDisplayFrame);
        }
        testNow=start+powerOnDuration(); robot.update();
        char status[512]; robot.describeStatus(status,sizeof(status));
        assert(std::string(status).find("animation=IDLE")!=std::string::npos);
        robot.update(); assert(lastDisplayFrame==expected.data());
    }
    // Time configured during boot takes effect only after the arrival finishes.
    reset(); Robot night; assert(night.begin()); night.setLocalTime(23*3600);
    assert(night.state()==RobotState::IDLE);
    finishStartup(night); assert(night.state()==RobotState::SLEEP);
    std::cout<<"PASS: blank first frame, approved power-on pixels/timing, input and schedule priority, restart and rollover\n";
}
void verifyTouchReturn(const desktoprobot::Frame& frame) {
    desktoprobot::Framebuffer released, start, end, tired;
    desktoprobot::drawFrame(frame,released);
    const auto& recovery=desktoprobot::getTouchReturnAnimation(frame);
    assert(!recovery.loop && recovery.frameCount>1);
    desktoprobot::drawFrame(recovery.frames[0],start);
    desktoprobot::drawFrame(recovery.frames[recovery.frameCount-1],end);
    desktoprobot::drawFrame(desktoprobot::getStateAnimation(desktoprobot::RobotState::Tired).frames[0],tired);
    assert(released.data()==start.data() && end.data()==tired.data());
}
int main() {
    testBlinkScheduling();
    testActivitySelection();
    testPowerOn();
    testSoundManager();
    testSoundBehaviour<FakeOutput>();
    reset(); InputController input; input.begin();
    tick(input,10,1); tick(input,25,0); tick(input,70,0); expect(input,InputEvent::NONE);
    tick(input,100,1); tick(input,135,1); tick(input,200,0); tick(input,235,0); expect(input,InputEvent::TAP);
    tick(input,300,1); tick(input,335,1); tick(input,999,1); expect(input,InputEvent::NONE);
    tick(input,1000,1); expect(input,InputEvent::HOLD);
    tick(input,2500,1); expect(input,InputEvent::NONE);
    tick(input,2600,0); tick(input,2635,0); expect(input,InputEvent::RELEASE);
    tick(input,3000,1); tick(input,3035,1); tick(input,3699,0); tick(input,3734,0); expect(input,InputEvent::TAP);
    tick(input,5000,1); tick(input,5035,1); tick(input,6100,0); expect(input,InputEvent::HOLD);
    // Even a threshold missed by a slow loop must provide HOLD then RELEASE.
    tick(input,6135,0); expect(input,InputEvent::RELEASE);
    tick(input,6200,0); expect(input,InputEvent::NONE);
    // Multiple complete taps during a blocked display update retain their
    // original timing and are delivered in order, without a repeat lockout.
    reset(); input.begin();
    testTouchEdge(100,1); testTouchEdge(180,0);
    testTouchEdge(240,1); testTouchEdge(330,0);
    testTouchEdge(400,1); testTouchEdge(490,0);
    assert(input.touchPending()); input.update(600);
    InputEvent queued;
    for(unsigned i=0;i<3;++i) { assert(input.pollEvent(queued) && queued==InputEvent::TAP); }
    expect(input,InputEvent::NONE);
    // Debounce still rejects noise and preserves a short tap near HOLD's threshold.
    testTouchEdge(700,1); testTouchEdge(710,0); testNow=750; input.update(testNow); expect(input,InputEvent::NONE);
    testTouchEdge(800,1); testTouchEdge(1499,0); testNow=1550; input.update(testNow); expect(input,InputEvent::TAP);
    // HOLD and RELEASE cannot overwrite one another in a delayed loop.
    testTouchEdge(1600,1); testTouchEdge(2400,0); testNow=2500; input.update(testNow);
    assert(input.pollEvent(queued) && queued==InputEvent::HOLD);
    assert(input.pollEvent(queued) && queued==InputEvent::RELEASE); expect(input,InputEvent::NONE);
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
        if(state==RobotState::TIRED) assert(out.openings==1);
        if(state==RobotState::SLEEP) assert(out.openings==1);
        if(state!=RobotState::IDLE) { out.done=true; brain.update(++t); }
        assert(out.last()==AnimationId::Angry);
        assert(out.hold==(state==RobotState::SLEEP));
        complete(brain,out,t);
        assert(brain.state()==(state==RobotState::SLEEP ? RobotState::TIRED : RobotState::IDLE));
    }
    for (auto state : {RobotState::IDLE,RobotState::TIRED,RobotState::SLEEP}) {
        FakeOutput out; BehaviourProgram brain(out); brain.enableSpontaneous(false); uint32_t t=0; brain.begin();
        brain.setState(state); complete(brain,out,t); brain.handleEvent(InputEvent::HOLD);
        assert(out.openings==0 && out.last()==AnimationId::LookUp && out.gazeSource==state);
        for(unsigned i=0;i<5;++i) { out.done=true; brain.update(++t); }
        assert(out.last()==AnimationId::Smile && out.hold && brain.busy());
        auto count=out.history.size(); out.done=true; t+=100000; brain.update(t); assert(out.history.size()==count);
        brain.handleEvent(InputEvent::RELEASE);
        assert(out.returns==(state==RobotState::SLEEP ? 1U : 0U)); complete(brain,out,t);
        assert(brain.state()==(state==RobotState::SLEEP ? RobotState::TIRED : RobotState::IDLE));
    }
    // Release during the slow hold gesture cancels the remaining smile.
    { FakeOutput out; BehaviourProgram brain(out); uint32_t t=0; brain.begin(); brain.setState(RobotState::SLEEP); complete(brain,out,t);
      out.history.clear(); brain.handleEvent(InputEvent::HOLD); brain.handleEvent(InputEvent::RELEASE); complete(brain,out,t);
      assert(out.history.size()==1 && out.last()==AnimationId::LookUp && out.openings==0); }
    // A second interaction during opening changes the tail without restarting wake-up.
    { FakeOutput out; BehaviourProgram brain(out); uint32_t t=0; brain.begin(); brain.setState(RobotState::SLEEP); complete(brain,out,t);
      out.history.clear(); brain.handleEvent(InputEvent::TAP); brain.handleEvent(InputEvent::HOLD);
      assert(out.history.empty() && out.openings==1);
      brain.handleEvent(InputEvent::RELEASE); complete(brain,out,t);
      assert(out.history.empty() && out.openings==1); }
    // Both touch events replace spontaneous playback immediately.
    { FakeOutput out; BehaviourProgram brain(out); brain.begin(); brain.preview(AnimationId::Thinking);
      brain.handleEvent(InputEvent::TAP); assert(out.last()==AnimationId::Angry);
      brain.preview(AnimationId::LookAround); brain.handleEvent(InputEvent::HOLD); assert(out.last()==AnimationId::LookUp); }
    // A pending press blocks due random activity AND blinking before it can be
    // classified. Reactions then keep priority even with every timer overdue.
    for(auto event:{InputEvent::TAP,InputEvent::HOLD,InputEvent::SHAKE_START}) {
      FakeOutput out; BehaviourConfig c;
      c.activityMinMs[0]=c.activityMaxMs[0]=100;
      c.blinkMinMs[0]=c.blinkMaxMs[0]=50; c.quietWeight=0;
      BehaviourProgram brain(out,c); brain.begin(); brain.setTouchPending(true);
      brain.update(100); assert(!brain.busy() && out.history.empty());
      brain.setTouchPending(false); brain.handleEvent(event);
      const auto expected=event==InputEvent::TAP ? AnimationId::Angry : event==InputEvent::HOLD ? AnimationId::LookUp : AnimationId::Shaking;
      assert(out.last()==expected); const auto count=out.history.size();
      brain.update(100000); assert(out.history.size()==count && out.last()==expected);
    }
    // All classified interactions interrupt an active blink or random activity.
    for(auto event:{InputEvent::TAP,InputEvent::HOLD,InputEvent::SHAKE_START}) {
      for(bool blink:{false,true}) {
        FakeOutput out; BehaviourConfig c;
        c.activityMinMs[0]=c.activityMaxMs[0]=blink ? 10000 : 100;
        c.blinkMinMs[0]=c.blinkMaxMs[0]=blink ? 100 : 10000; c.quietWeight=0;
        BehaviourProgram brain(out,c); brain.begin(); brain.update(100); assert(brain.busy());
        brain.handleEvent(event);
        assert(out.last()==(event==InputEvent::TAP ? AnimationId::Angry : event==InputEvent::HOLD ? AnimationId::LookUp : AnimationId::Shaking));
      }
    }
    // Even the first raw press sample cancels an existing blink before HOLD.
    { FakeOutput out; BehaviourConfig c; c.blinkMinMs[0]=c.blinkMaxMs[0]=100;
      BehaviourProgram brain(out,c); brain.begin(); brain.update(100); assert(out.last()==AnimationId::Blink);
      brain.setTouchPending(true); brain.update(101); assert(!brain.busy());
      brain.update(100000); assert(!brain.busy() && out.history.size()==1); }
    // Early waking cannot expire back to sleep before 07:00.
    { FakeOutput out; BehaviourProgram brain(out); brain.enableSpontaneous(false); uint32_t t=0;
      brain.begin(t,clockAt(6)); complete(brain,out,t,clockAt(6)); brain.handleEvent(InputEvent::TAP); complete(brain,out,t,clockAt(6));
      assert(brain.state()==RobotState::TIRED); brain.update(t+600000,clockAt(6,10)); assert(brain.state()==RobotState::TIRED);
      brain.update(t+3600000,clockAt(7)); assert(brain.state()==RobotState::IDLE && out.last()==AnimationId::TiredToIdle); }
    // TAP and HOLD use the same temporary waking timers.
    for(auto event:{InputEvent::TAP,InputEvent::HOLD}) {
    { FakeOutput out; BehaviourProgram brain(out); brain.enableSpontaneous(false); uint32_t t=0;
      brain.begin(t,clockAt(22)); complete(brain,out,t,clockAt(22)); brain.handleEvent(event);
      if(event==InputEvent::HOLD) brain.handleEvent(InputEvent::RELEASE);
      complete(brain,out,t,clockAt(22)); assert(brain.state()==RobotState::TIRED);
      brain.update(t+300000,clockAt(22,5)); assert(brain.state()==RobotState::SLEEP); }
    { FakeOutput out; BehaviourProgram brain(out); brain.enableSpontaneous(false); uint32_t t=0;
      brain.begin(t,clockAt(21,5)); complete(brain,out,t,clockAt(21,5)); brain.handleEvent(event);
      if(event==InputEvent::HOLD) brain.handleEvent(InputEvent::RELEASE);
      complete(brain,out,t,clockAt(21,5)); assert(brain.state()==RobotState::IDLE);
      brain.update(t+300000,clockAt(21,10)); assert(brain.state()==RobotState::TIRED);
      t+=300000; complete(brain,out,t,clockAt(21,10)); brain.update(t+300000,clockAt(21,15)); assert(brain.state()==RobotState::SLEEP); }
    }
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
    { FakeOutput out; BehaviourConfig c; c.spontaneousGapMinMs=c.spontaneousGapMaxMs=0; c.randomSeed=19; c.activityMinMs[0]=c.activityMaxMs[0]=10000;
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
        bool allowed = (state==RobotState::SLEEP && id==AnimationId::SleepEyeOpen)
            || (state==RobotState::TIRED && id==AnimationId::SlowBlink);
        for(const auto& a:c.activities) if(a.state==state && a.id==id) allowed=true;
        assert(allowed);
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
    // RELEASE during the slow LOOK_UP cancels the remaining smile in every state.
    for(auto state:{RobotState::IDLE,RobotState::TIRED,RobotState::SLEEP}) {
      FakeOutput out; BehaviourProgram brain(out); uint32_t t=0; brain.begin();
      brain.setState(state); complete(brain,out,t); brain.handleEvent(InputEvent::HOLD);
      assert(out.last()==AnimationId::LookUp);
      auto size=out.history.size(); brain.handleEvent(InputEvent::RELEASE);
      complete(brain,out,t); assert(out.history.size()==size);
      assert(brain.state()==(state==RobotState::SLEEP ? RobotState::TIRED : RobotState::IDLE));
    }
    // TAP retains its exact quick wake timing.
    for(auto state:{RobotState::TIRED,RobotState::SLEEP}) {
      reset(); OutputDisplay d; assert(d.begin()); d.playTouchWake(state,0);
      d.update(219); assert(!d.isAnimationFinished()); d.update(220); assert(d.isAnimationFinished());
    }
    // Every hold starts at its source face, opens before gazing upward, then
    // joins the unchanged original gaze and the neutral start of Smile exactly.
    for(auto state:{RobotState::IDLE,RobotState::TIRED,RobotState::SLEEP}) {
      reset(); OutputDisplay d; assert(d.begin());
      desktoprobot::Framebuffer expectedStart, expectedEnd;
      desktoprobot::drawFrame(desktoprobot::getStateAnimation(static_cast<desktoprobot::RobotState>(state)).frames[0],expectedStart);
      desktoprobot::drawFrame(getAnimation(AnimationId::Smile).frames[0],expectedEnd);
      d.playTouchLookUp(state,0);
      const auto& gesture=*d.currentAnimation();
      desktoprobot::Framebuffer actualStart, actualEnd;
      desktoprobot::drawFrame(gesture.frames[0],actualStart);
      desktoprobot::drawFrame(gesture.frames[gesture.frameCount-1],actualEnd);
      assert(actualStart.data()==expectedStart.data() && actualEnd.data()==expectedEnd.data());
      uint32_t duration=0;
      for(std::size_t i=0;i<gesture.frameCount;++i) { assert(gesture.frames[i].durationMs>0); duration+=gesture.frames[i].durationMs; }
      const std::size_t offset=state==RobotState::IDLE ? 0 : 8;
      // Save frames before requesting another animation: ESP32 reuses buffers.
      std::vector<desktoprobot::Framebuffer> gazeFrames;
      std::vector<uint32_t> gazeDurations;
      for(std::size_t i=offset;i<gesture.frameCount;++i) {
        desktoprobot::Framebuffer frame; desktoprobot::drawFrame(gesture.frames[i],frame);
        gazeFrames.push_back(frame); gazeDurations.push_back(gesture.frames[i].durationMs);
      }
      d.update(duration-1); assert(!d.isAnimationFinished());
      d.update(duration); assert(d.isAnimationFinished() && d.framebuffer().data()==expectedEnd.data());
      assert(duration>2000 && (state==RobotState::IDLE || duration>2700));
      // The suffix has exactly the historical LookUp frame count.
      assert(gesture.frameCount-offset==33);
      const auto& original=getAnimation(AnimationId::LookUp);
      assert(original.frameCount==gazeFrames.size());
      for(std::size_t i=0;i<original.frameCount;++i) {
        desktoprobot::Framebuffer expected; desktoprobot::drawFrame(original.frames[i],expected);
        assert(expected.data()==gazeFrames[i].data() && original.frames[i].durationMs==gazeDurations[i]);
      }
    }
    // Returning to TIRED starts at the exact displayed pixels, even when HOLD
    // is released halfway through opening, gazing or smiling.
    for(auto id:{AnimationId::Angry,AnimationId::Smile}) {
      for(std::size_t i=0;i<desktoprobot::getExpressionFrameCount(id);++i)
        verifyTouchReturn(getAnimation(id).frames[i]);
    }
    for(auto source:{desktoprobot::RobotState::Tired,desktoprobot::RobotState::Sleep}) {
      for(std::size_t i=0;i<41;++i) verifyTouchReturn(getTouchWakeAnimation(source,true).frames[i]);
      for(std::size_t i=0;i<9;++i) verifyTouchReturn(getTouchWakeAnimation(source).frames[i]);
    }
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
    testTimeManager();
    testNtpSchedule();
    testConsole();
    std::cout<<"PASS: schedule, six transitions, touch/release, temporary wakes, priorities, shake/recovery, activities, hardware diagnostics\n";
}

void testConsole() {
#if ROBOT_DEBUG_CONSOLE
    const auto pump=[](Robot& robot,unsigned loops=60) { for(unsigned i=0;i<loops;++i) robot.update(); };
    const auto command=[&](Robot& robot,const std::string& line) { Serial.feed(line+"\n"); pump(robot); };
    const auto advance=[&](Robot& robot,unsigned ms) { for(unsigned i=0;i<ms/50;++i) { testNow+=50; robot.update(); } pump(robot); };
    const auto status=[](Robot& robot) { char text[512]; robot.describeStatus(text,sizeof(text)); return std::string(text); };
    const auto occurrences=[](const std::string& text,const std::string& needle) {
        unsigned count=0; std::size_t pos=0; while((pos=text.find(needle,pos))!=std::string::npos) { ++count; pos+=needle.size(); } return count;
    };
    reset(); Robot robot; assert(robot.begin()); finishStartup(robot); pump(robot);
    command(robot,"help"); assert(Serial.log.find("Commands:")!=std::string::npos);
    command(robot,"status"); assert(Serial.log.find("AUTO")!=std::string::npos && Serial.log.find("UNSET")!=std::string::npos);
    command(robot,"sleep"); assert(robot.state()==RobotState::SLEEP); advance(robot,15000);
    Serial.log.clear(); command(robot,"wake");
    assert(status(robot).find("WAKE UP")!=std::string::npos);
    advance(robot,18000); assert(robot.state()==RobotState::IDLE);
    assert(Serial.log.find("started YAWN")!=std::string::npos && Serial.log.find("started LOOK AROUND")!=std::string::npos);
    command(robot,"tired"); advance(robot,15000); assert(robot.state()==RobotState::TIRED);
    command(robot,"tap"); advance(robot,15000); assert(robot.state()==RobotState::IDLE);
    assert(status(robot).find("IDLE_THEN_TIRED")!=std::string::npos);
    command(robot,"time auto"); advance(robot,15000); assert(status(robot).find("temporary=NONE")!=std::string::npos);
    command(robot,"hold"); advance(robot,15000); assert(status(robot).find("held=yes")!=std::string::npos);
    assert(status(robot).find("animation=SMILE")!=std::string::npos);
    command(robot,"release"); advance(robot,1000); assert(status(robot).find("held=no")!=std::string::npos);
    assert(robot.state()==RobotState::IDLE);
    command(robot,"shake 3000"); assert(status(robot).find("animation=SHAKING")!=std::string::npos);
    advance(robot,2950); assert(status(robot).find("animation=SHAKING")!=std::string::npos);
    advance(robot,50); assert(status(robot).find("animation=DIZZY")!=std::string::npos);
    command(robot,"shake 100"); assert(status(robot).find("animation=SHAKING")!=std::string::npos);
    advance(robot,10000); assert(robot.state()==RobotState::IDLE);
    // Separate AUTO time continues to advance while simulation is active.
    robot.setLocalTime(20*3600+59*60+59); command(robot,"time 06:59"); advance(robot,15000);
    assert(robot.state()==RobotState::SLEEP && status(robot).find("SIMULATED")!=std::string::npos);
    advance(robot,45000); assert(robot.state()==RobotState::IDLE);
    const auto wakes=occurrences(Serial.log,"started WAKE UP");
    command(robot,"time 07:00"); command(robot,"time 07:00");
    assert(occurrences(Serial.log,"started WAKE UP")==wakes);
    command(robot,"time auto"); assert(robot.state()==RobotState::TIRED);
    assert(status(robot).find("mode=AUTO")!=std::string::npos);
    const auto transitions=occurrences(Serial.log,"started IDLE TO TIRED");
    command(robot,"time auto"); assert(occurrences(Serial.log,"started IDLE TO TIRED")==transitions);
    advance(robot,15000);
    command(robot,"time 21:30"); assert(robot.state()==RobotState::SLEEP);
    advance(robot,15000); command(robot,"time 07:00"); assert(robot.state()==RobotState::IDLE);
    advance(robot,18000); command(robot,"time 21:00"); assert(robot.state()==RobotState::TIRED);
    // Invalid input, partial lines, CRLF, and overlong input never execute a suffix.
    advance(robot,15000); const auto state=robot.state();
    for(const auto& line:{"time 24:00","time 7:00","time -1","shake -5","shake 0","shake 600001","tap extra","unknown"}) command(robot,line);
    assert(robot.state()==state);
    Serial.feed("sle"); pump(robot); assert(robot.state()==state);
    Serial.feed("ep\r\n"); pump(robot); assert(robot.state()==RobotState::SLEEP); advance(robot,15000);
    command(robot,std::string(110,'x')+"wake"); assert(robot.state()==RobotState::SLEEP);
    command(robot,"state"); command(robot,"diag"); assert(Serial.log.find("Startup: OK")!=std::string::npos);
    // A stalled USB host cannot block playback or a scheduled SHAKE_END.
    command(robot,"time auto"); advance(robot,15000); Serial.txCapacity=0;
    command(robot,"shake 100"); advance(robot,10000);
    assert(status(robot).find("animation=SHAKING")==std::string::npos);
    for(unsigned i=0;i<30;++i) command(robot,"status");
    Serial.txCapacity=64; pump(robot,150); assert(Serial.log.find("Output overflow")!=std::string::npos);
    // Elapsed shake duration remains correct across millis wraparound.
    reset(); testNow=UINT32_MAX-powerOnDuration()-50; Robot wrapped; assert(wrapped.begin()); finishStartup(wrapped); pump(wrapped);
    command(wrapped,"shake 100"); advance(wrapped,100);
    assert(status(wrapped).find("animation=DIZZY")!=std::string::npos);
    // Simulated tap and the physical input reach exactly the same behavior path.
    reset(); Robot physical; assert(physical.begin()); finishStartup(physical); pump(physical);
    testPin=1; physical.update(); testNow+=35; physical.update();
    testPin=0; physical.update(); testNow+=35; physical.update(); pump(physical);
    assert(status(physical).find("animation=ANGRY")!=std::string::npos);
    assert(Serial.log.find("[EVENT] TAP received")!=std::string::npos);
    // A complete tap between loop iterations is accepted during ANGRY and
    // immediately after its end, restarting the reaction without a dead zone.
    reset(); Robot repeated; assert(repeated.begin()); finishStartup(repeated); pump(repeated);
    const auto shortTap=[&]() {
      const auto at=testNow+1;
      testTouchEdge(at,1); testTouchEdge(at+80,0);
      testNow=at+120; repeated.update(); pump(repeated);
    };
    shortTap(); assert(occurrences(Serial.log,"started ANGRY")==1);
    shortTap(); assert(occurrences(Serial.log,"started ANGRY")==2);
    for(unsigned i=0;i<100 && status(repeated).find("mode=REACTION")!=std::string::npos;++i) advance(repeated,50);
    assert(status(repeated).find("mode=BASE")!=std::string::npos);
    shortTap(); assert(occurrences(Serial.log,"started ANGRY")==3);
    assert(status(repeated).find("animation=ANGRY")!=std::string::npos);
    // Physical press wins when spontaneous playback becomes due and
    // while an activity is already playing; release still delivers a full ANGRY.
    for(bool alreadyPlaying:{false,true}) {
      reset(); Robot priority; assert(priority.begin()); finishStartup(priority); pump(priority);
      if(alreadyPlaying) {
        for(unsigned i=0;i<800 && status(priority).find("mode=ACTIVITY")==std::string::npos;++i)
          advance(priority,50);
        assert(status(priority).find("mode=ACTIVITY")!=std::string::npos);
        pump(priority); // Drain the buffered start log before counting new blinks.
        ++testNow;
      } else testNow+=6000;
      const auto blinks=occurrences(Serial.log,"started BLINK");
      testPin=1; priority.update();
      assert(status(priority).find("animation=IDLE")!=std::string::npos);
      testNow+=35; priority.update(); testNow+=100;
      testPin=0; priority.update(); testNow+=35; priority.update(); pump(priority);
      assert(status(priority).find("animation=ANGRY")!=std::string::npos);
      advance(priority,500); assert(status(priority).find("animation=ANGRY")!=std::string::npos);
      assert(occurrences(Serial.log,"started BLINK")==blinks);
    }
    reset(); Wire.ack=2; Robot error; assert(!error.begin()); pump(error);
    const auto reads=testReads;
    for(const auto& line:{"help","status","state","diag","wake","tired","sleep","tap","hold","release","shake 3000","time 07:00","time auto"}) command(error,line);
    assert(error.state()==RobotState::ERROR && testReads==reads && sentFrames==0);
    assert(Serial.log.find("Display not responding")!=std::string::npos);
    assert(Serial.log.find("Rejected: ERROR")!=std::string::npos);
    std::cout<<"PASS: console commands, sequences, serial parsing/backpressure, clock modes/boundaries, real/simulated event path, ERROR and rollover\n";
#endif
}

void testNtpSchedule() {
    FakeTimeNetwork network; TimeConfig config; config.ssid="test-only";
    TimeManager manager(network,config); manager.begin(0);
    network.link=TimeNetwork::Connection::Connected; manager.update(1);
    network.epoch=1768456799; // 2026-01-15 05:59:59 UTC == 06:59:59 CET
    network.notified=true; manager.update(2);
    assert(manager.localTime().second==25199);
    FakeOutput output; BehaviourProgram brain(output); brain.enableSpontaneous(false);
    uint32_t tick=2; brain.begin(tick,manager.localTime()); complete(brain,output,tick,manager.localTime());
    assert(brain.state()==RobotState::SLEEP);
    ++network.epoch; manager.update(++tick); brain.update(tick,manager.localTime());
    assert(brain.state()==RobotState::IDLE && output.last()==AnimationId::WakeUp);
    complete(brain,output,tick,manager.localTime());
    network.epoch+=14*3600; manager.update(++tick); brain.update(tick,manager.localTime());
    assert(brain.state()==RobotState::TIRED && output.last()==AnimationId::IdleToTired);
    complete(brain,output,tick,manager.localTime());
    network.epoch+=1800; manager.update(++tick); brain.update(tick,manager.localTime());
    assert(brain.state()==RobotState::SLEEP && output.last()==AnimationId::TiredToSleep && manager.radioOff());
    std::cout<<"PASS: NTP local clock drives 07:00, 21:00 and 21:30 behavior transitions offline\n";
}
