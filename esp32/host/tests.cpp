#include <Arduino.h>
#include <Wire.h>
#include <cassert>
#include <iostream>
#include <vector>
#include "Robot.h"
#include "TimeManagerTests.h"
#include "display/animations/AnimationCatalog.h"

struct FakeOutput : AnimationOutput {
    std::vector<AnimationId> history;
    RobotState baseState = RobotState::IDLE;
    bool done = false, error = false, hold = false;
    unsigned bases = 0, recoveries = 0;
    void playAnimation(AnimationId id, uint32_t, bool held = false) override { history.push_back(id); done=false; hold=held; }
    unsigned openings=0;
    void playTouchWake(RobotState,uint32_t) override { ++openings; done=false; }
    void playTouchLookUp(uint32_t now) override { playAnimation(AnimationId::LookUp,now); }
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
        if(state==RobotState::TIRED) assert(out.openings==1);
        if(state==RobotState::SLEEP) assert(out.openings==1);
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
      assert(out.history.empty() && out.openings==1); }
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
    // RELEASE during the short LOOK_UP cancels the remaining smile in every state.
    for(auto state:{RobotState::IDLE,RobotState::TIRED,RobotState::SLEEP}) {
      FakeOutput out; BehaviourProgram brain(out); uint32_t t=0; brain.begin();
      brain.setState(state); complete(brain,out,t); brain.handleEvent(InputEvent::HOLD);
      if(state!=RobotState::IDLE) { out.done=true; brain.update(t+=220); }
      assert(out.last()==AnimationId::LookUp);
      auto size=out.history.size(); brain.handleEvent(InputEvent::RELEASE);
      assert(!brain.busy() && out.history.size()==size);
      assert(brain.state()==(state==RobotState::SLEEP ? RobotState::TIRED : state));
    }
    // Quick wake and gaze timing are exact and independent of update frequency.
    for(auto state:{RobotState::TIRED,RobotState::SLEEP}) {
      reset(); OutputDisplay d; assert(d.begin()); d.playTouchWake(state,0);
      d.update(219); assert(!d.isAnimationFinished()); d.update(220); assert(d.isAnimationFinished());
      d.playTouchLookUp(220); d.update(419); assert(!d.isAnimationFinished()); d.update(420); assert(d.isAnimationFinished());
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
    reset(); Robot robot; assert(robot.begin()); pump(robot);
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
    reset(); testNow=UINT32_MAX-50; Robot wrapped; assert(wrapped.begin()); pump(wrapped);
    command(wrapped,"shake 100"); advance(wrapped,100);
    assert(status(wrapped).find("animation=DIZZY")!=std::string::npos);
    // Simulated tap and the physical input reach exactly the same behavior path.
    reset(); Robot physical; assert(physical.begin()); pump(physical);
    testPin=1; physical.update(); testNow+=35; physical.update();
    testPin=0; physical.update(); testNow+=35; physical.update(); pump(physical);
    assert(status(physical).find("animation=ANGRY")!=std::string::npos);
    assert(Serial.log.find("[EVENT] TAP received")!=std::string::npos);
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
