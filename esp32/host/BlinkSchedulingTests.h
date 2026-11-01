#pragma once

inline void testBlinkScheduling() {
    for(const auto state:{RobotState::IDLE,RobotState::TIRED}) {
        for(uint32_t beginAt:{0U,UINT32_MAX-100}) {
            FakeOutput out; BehaviourConfig c; c.quietWeight=0; c.spontaneousGapMinMs=c.spontaneousGapMaxMs=20;
            for(auto& a:c.activities) a.weight=0;
            c.activities[state==RobotState::IDLE ? 0:10].weight=1;
            for(unsigned i=0;i<3;++i) c.activityMinMs[i]=c.activityMaxMs[i]=40;
            for(unsigned i=0;i<2;++i) c.blinkMinMs[i]=c.blinkMaxMs[i]=100;
            BehaviourProgram brain(out,c,[](void*)->uint32_t { return 0; });
            uint32_t now=beginAt; brain.begin(now); brain.setState(state); complete(brain,out,now);
            const auto origin=now;
            const auto blink=state==RobotState::IDLE ? AnimationId::Blink:AnimationId::SlowBlink;
            out.history.clear(); brain.update(origin+40);
            assert(brain.busy() && out.last()!=blink);
            const auto count=out.history.size();
            brain.update(origin+120); assert(out.history.size()==count); // Do not interrupt a gesture.
            out.done=true; brain.update(origin+140);
            assert(out.history.size()==count); // Never chain a blink straight onto an activity.
            brain.update(origin+159); assert(out.history.size()==count);
            brain.update(origin+160); assert(out.last()==blink && out.history.size()==count+1);
            out.done=true; brain.update(origin+180);
            brain.update(origin+200); brain.update(origin+240); // Quiet decisions while on cooldown.
            const auto before=out.history.size();
            brain.update(origin+280);
            assert(out.last()==blink && out.history.size()==before+1);
        }
    }
    // If deadlines coincide, the random draw can choose either kind. The
    // pending activity must still wait for the breathing gap after a blink.
    {
        FakeOutput out; BehaviourConfig c; c.quietWeight=0; c.spontaneousGapMinMs=c.spontaneousGapMaxMs=20;
        c.activityMinMs[0]=c.activityMaxMs[0]=100;
        c.blinkMinMs[0]=c.blinkMaxMs[0]=100;
        BehaviourProgram brain(out,c,[](void*)->uint32_t { return 0; }); brain.begin(); brain.update(100);
        assert(out.last()==AnimationId::Blink);
        out.done=true; brain.update(101);
        assert(!brain.busy() && out.history.size()==1);
        brain.update(120); assert(out.history.size()==1);
        brain.update(121); assert(brain.busy() && out.last()!=AnimationId::Blink && out.history.size()==2);
    }
    {
        FakeOutput out; BehaviourConfig c; c.quietWeight=0;
        c.activityMinMs[0]=c.activityMaxMs[0]=100;
        c.blinkMinMs[0]=c.blinkMaxMs[0]=100;
        BehaviourProgram brain(out,c,[](void*)->uint32_t { return 1; });
        brain.begin(); brain.update(100);
        assert(brain.busy() && out.last()!=AnimationId::Blink);
    }
    {
        FakeOutput out; BehaviourConfig c; c.quietWeight=0;
        c.activityMinMs[0]=c.activityMaxMs[0]=40;
        c.blinkMinMs[0]=c.blinkMaxMs[0]=100;
        BehaviourProgram brain(out,c); brain.begin(); brain.update(140);
        assert(brain.busy() && out.last()!=AnimationId::Blink); // Older activity wins.
    }
    // Replay real animation lengths for ten simulated minutes. Independent
    // random clocks must produce repeated blinks, rather than strict alternation,
    // while every pair of gestures retains a visible breathing gap.
    {
        FakeOutput out; BehaviourConfig c; c.randomSeed=0xC0FFEE;
        BehaviourProgram brain(out,c); brain.begin();
        uint32_t finish=0,previousFinish=0;
        std::size_t seen=0; unsigned blinks=0,activities=0; bool previousBlink=false,repeatedBlink=false;
        for(uint32_t now=50;now<=600000;now+=50) {
            if(brain.busy() && now>=finish) { out.done=true; previousFinish=now; }
            brain.update(now);
            if(out.history.size()==seen) continue;
            assert(out.history.size()==seen+1);
            if(seen) assert(now-previousFinish>=c.spontaneousGapMinMs);
            const auto id=out.last(); const bool blink=id==AnimationId::Blink;
            if(blink) { ++blinks; if(previousBlink) repeatedBlink=true; } else ++activities;
            previousBlink=blink; seen=out.history.size();
            const auto& animation=desktoprobot::getTestAnimation(id);
            finish=now;
            for(std::size_t i=0;i<animation.frameCount;++i) finish+=animation.frames[i].durationMs;
        }
        assert(blinks>10 && activities>10 && repeatedBlink);
    }
    std::cout<<"PASS: overdue IDLE/TIRED blinks survive activities and quiet draws, wait between gestures and preserve activity timing across rollover\n";
}
