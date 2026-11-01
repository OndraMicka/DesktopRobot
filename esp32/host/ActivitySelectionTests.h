#pragma once

inline void testActivitySelection() {
    // Enumerate every weighted bucket, including quiet and the special sleep peek.
    // This proves each animation is reachable with the intended relative weight.
    const BehaviourConfig defaults;
    for(const auto state:{RobotState::IDLE,RobotState::TIRED,RobotState::SLEEP}) {
        unsigned total=defaults.quietWeight;
        for(const auto& a:defaults.activities) if(a.state==state) total+=a.weight;
        if(state==RobotState::SLEEP) total+=defaults.sleepPeekWeight;
        assert(total==(state==RobotState::IDLE ? 142U:state==RobotState::TIRED ? 146U:40U));
        for(unsigned bucket=0;bucket<total;++bucket) {
            FakeOutput out; auto c=defaults; c.spontaneousGapMinMs=c.spontaneousGapMaxMs=0;
            for(unsigned i=0;i<3;++i) c.activityMinMs[i]=c.activityMaxMs[i]=1;
            for(unsigned i=0;i<2;++i) c.blinkMinMs[i]=c.blinkMaxMs[i]=1000000;
            BehaviourProgram brain(out,c,[](void* context) { return *static_cast<unsigned*>(context); },&bucket);
            uint32_t now=0; brain.begin(); brain.setState(state); complete(brain,out,now);
            out.history.clear(); brain.update(++now);
            if(bucket<c.quietWeight) { assert(out.history.empty()); continue; }
            unsigned choice=bucket-c.quietWeight;
            auto expected=AnimationId::SleepEyeOpen;
            for(const auto& a:c.activities) {
                if(a.state!=state) continue;
                if(choice<a.weight) { expected=a.id; break; }
                choice-=a.weight;
            }
            assert(out.history.size()==1 && out.last()==expected);
        }
    }
    // Each promoted animation observes its own cooldown and can play again at
    // the exact boundary after another activity, independent of weighted chance.
    for(const auto& target:defaults.activities) {
        if(target.id<AnimationId::Microsleep) continue;
        FakeOutput out; auto c=defaults; c.spontaneousGapMinMs=c.spontaneousGapMaxMs=0; c.quietWeight=0; c.sleepPeekWeight=0;
        for(auto& a:c.activities) a.weight=0;
        const unsigned filler=target.state==RobotState::IDLE ? 0:target.state==RobotState::TIRED ? 10:12;
        c.activities[filler].weight=1; c.activities[filler].cooldownMs=0;
        for(auto& a:c.activities) if(a.id==target.id) a.weight=1;
        for(unsigned i=0;i<3;++i) c.activityMinMs[i]=c.activityMaxMs[i]=1;
        for(unsigned i=0;i<2;++i) c.blinkMinMs[i]=c.blinkMaxMs[i]=1000000;
        BehaviourProgram brain(out,c,[](void*)->uint32_t { return 1; });
        uint32_t now=0; brain.begin(); brain.setState(target.state); complete(brain,out,now);
        out.history.clear(); const uint32_t started=++now; brain.update(now);
        assert(out.last()==target.id); complete(brain,out,now);
        brain.update(++now); assert(out.last()==c.activities[filler].id); complete(brain,out,now);
        const auto count=out.history.size();
        brain.update(started+target.cooldownMs-1); assert(out.history.size()==count);
        brain.update(started+target.cooldownMs); assert(out.last()==target.id && out.history.size()==count+1);
    }
    // The other glance is blocked until the shared cooldown boundary. Check
    // the same timeline over millis rollover as well.
    for(uint32_t beginAt:{0U,UINT32_MAX-6000}) {
        FakeOutput out; auto c=defaults; c.spontaneousGapMinMs=c.spontaneousGapMaxMs=0; c.quietWeight=0; c.sleepPeekWeight=0;
        for(auto& a:c.activities) a.weight=0;
        c.activities[0].weight=1; c.activities[0].cooldownMs=0;
        for(auto& a:c.activities)
            if(a.id==AnimationId::StartleLeft || a.id==AnimationId::StartleUpRight) a.weight=1;
        c.activityMinMs[0]=c.activityMaxMs[0]=1;
        c.blinkMinMs[0]=c.blinkMaxMs[0]=1000000;
        unsigned choice=1;
        BehaviourProgram brain(out,c,[](void* context) { return *static_cast<unsigned*>(context); },&choice);
        uint32_t now=beginAt; brain.begin(now); const uint32_t started=++now;
        brain.update(now); assert(out.last()==AnimationId::StartleLeft); complete(brain,out,now);
        choice=0; brain.update(++now); assert(out.last()==AnimationId::LookAround); complete(brain,out,now);
        const auto count=out.history.size();
        brain.update(started+c.startleGroupCooldownMs-1); assert(out.history.size()==count);
        brain.update(started+c.startleGroupCooldownMs); assert(out.last()==AnimationId::StartleUpRight && out.history.size()==count+1);
    }
    std::cout<<"PASS: all weighted activity buckets, 10 promoted cooldowns, shared startle boundary and rollover\n";
}
