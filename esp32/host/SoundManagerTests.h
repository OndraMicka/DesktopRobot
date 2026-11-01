#pragma once
#include "sound/SoundManager.h"
#include "behavior/BehaviourProgram.h"
#include <cassert>
#include <iostream>
#include <vector>

struct FakeToneOutput : desktoprobot::ToneOutput {
    bool initialized = true, writable = true;
    std::vector<uint16_t> tones;
    bool begin() override { return initialized; }
    bool setFrequency(uint16_t hz) override { tones.push_back(hz); return writable; }
    uint16_t last() const { return tones.back(); }
};
struct FakeSoundOutput : desktoprobot::SoundOutput {
    struct Request { desktoprobot::SoundEffect effect; bool loop; };
    std::vector<Request> requests;
    bool playing = false;
    unsigned stops = 0;
    void play(desktoprobot::SoundEffect effect, bool loop = false) override {
        requests.push_back({effect,loop}); playing=true;
    }
    void stop() override { ++stops; playing=false; }
    bool isPlaying() const override { return playing; }
};

inline void testSoundManager() {
    using namespace desktoprobot;
    // Validate every immutable score, including all pauses and exact totals.
    for (unsigned id=0; id<static_cast<unsigned>(SoundEffect::Count); ++id) {
        const auto* data=getSoundData(static_cast<SoundEffect>(id));
        assert(data && data->count && data->durationMs);
        uint32_t sum=0;
        for (std::size_t i=0;i<data->count;++i) {
            assert(data->segments[i].durationMs); sum+=data->segments[i].durationMs;
        }
        assert(sum==data->durationMs);
    }
    assert(!getSoundData(SoundEffect::Count));
    assert(getSoundData(SoundEffect::Tap1)->durationMs==210);
    assert(getSoundData(SoundEffect::Shake1)->durationMs==260);

    FakeToneOutput tone; SoundManager manager(tone);
    manager.play(SoundEffect::Tap1); assert(!manager.isPlaying());
    assert(manager.begin() && manager.healthy() && tone.last()==0);
    manager.play(SoundEffect::Tap1); assert(manager.isPlaying());
    manager.update(1000); assert(tone.last()==659);
    manager.update(1064); assert(tone.last()==659);
    manager.update(1065); assert(tone.last()==0 && manager.isPlaying());
    manager.update(1110); assert(tone.last()==988);
    manager.update(1210); assert(tone.last()==0 && !manager.isPlaying());
    const auto writes=tone.tones.size(); manager.update(1500);
    assert(tone.tones.size()==writes); // Completed scores stay silent.

    manager.play(SoundEffect::Tap1); manager.update(2000);
    manager.update(2110); assert(tone.last()==988);
    manager.play(SoundEffect::Tap1); manager.update(2111);
    assert(tone.last()==659); // Same effect restarts immediately at next service.
    manager.play(SoundEffect::Hold); manager.update(2120); assert(tone.last()==392);
    manager.stop(); assert(!manager.isPlaying());
    manager.update(2121); assert(tone.last()==0);
    manager.update(10000); assert(!manager.isPlaying());

    manager.play(SoundEffect::Shake1,true); manager.update(20000);
    manager.update(20210); assert(tone.last()==0 && manager.isPlaying());
    manager.update(20260); assert(tone.last()==659);
    // A long service gap skips complete cycles without an unbounded loop.
    manager.update(20260+2600000+130); assert(tone.last()==988 && manager.isPlaying());
    manager.play(SoundEffect::Count); assert(manager.isPlaying()); // Invalid ID is ignored.
    manager.stop(); manager.update(3000000); assert(tone.last()==0);

    // Only the latest request wins, including stop before first service.
    manager.play(SoundEffect::Tap1); manager.play(SoundEffect::Hold);
    manager.update(3000010); assert(tone.last()==392);
    manager.play(SoundEffect::Tap1); manager.stop();
    manager.update(3000011); assert(tone.last()==0 && !manager.isPlaying());

    const uint32_t nearWrap=UINT32_MAX-40;
    manager.play(SoundEffect::Tap1); manager.update(nearWrap);
    manager.update(nearWrap+65); assert(tone.last()==0 && manager.isPlaying());
    manager.update(nearWrap+110); assert(tone.last()==988);
    manager.update(nearWrap+210); assert(tone.last()==0 && !manager.isPlaying());
    manager.play(SoundEffect::PowerOn); manager.update(1000);
    manager.update(100000); assert(tone.last()==0 && !manager.isPlaying());

    FakeToneOutput unavailable; unavailable.initialized=false;
    SoundManager missing(unavailable); assert(!missing.begin());
    missing.play(SoundEffect::Tap1); missing.update(0); assert(!missing.isPlaying());
    FakeToneOutput broken; SoundManager failed(broken); assert(failed.begin());
    broken.writable=false; failed.play(SoundEffect::Tap1); failed.update(0);
    assert(!failed.healthy() && !failed.isPlaying());
    const auto failures=broken.tones.size(); failed.play(SoundEffect::Hold); failed.update(1);
    assert(broken.tones.size()==failures); // No repeated hardware-error calls.

    // Every score finishes at its exact duration; every loop wraps identically.
    for (unsigned id=0;id<static_cast<unsigned>(SoundEffect::Count);++id) {
        const auto effect=static_cast<SoundEffect>(id);
        const auto& data=*getSoundData(effect);
        manager.play(effect); manager.update(1000);
        uint32_t at=1000;
        for (std::size_t i=0;i<data.count;++i) {
            assert(tone.last()==data.segments[i].frequencyHz && manager.isPlaying());
            at+=data.segments[i].durationMs; manager.update(at);
        }
        assert(!manager.isPlaying() && tone.last()==0);
        manager.play(effect,true); manager.update(1000);
        manager.update(1000+data.durationMs*10000);
        assert(manager.isPlaying() && tone.last()==data.segments[0].frequencyHz);
        manager.stop(); manager.update(1001+data.durationMs*10000);
    }
    std::cout<<"PASS: shared sound scores, tones/pauses, restart/replacement, loops, stop, rollover and output failures\n";
}

template<class Output> void testSoundBehaviour() {
    using desktoprobot::SoundEffect;
    Output output; FakeSoundOutput sound;
    BehaviourProgram behavior(output,sound);
    behavior.enableSpontaneous(false);
    uint32_t now=0;
    const auto finish=[&]() {
        for (unsigned i=0;i<12 && behavior.busy();++i) {
            output.done=true; behavior.update(++now);
        }
        assert(!behavior.busy());
    };
    behavior.begin(); assert(sound.requests.empty());
    behavior.handleEvent(InputEvent::TAP);
    assert(sound.requests.size()==1 && sound.requests.back().effect==SoundEffect::Tap1);
    behavior.update(++now); behavior.update(++now);
    assert(sound.requests.size()==1); // The two firmware updates don't retrigger.
    behavior.handleEvent(InputEvent::TAP); assert(sound.requests.size()==2);
    behavior.handleEvent(InputEvent::HOLD);
    assert(sound.requests.size()==2 && !sound.playing); // Opening/gaze stays silent.
    behavior.update(++now); assert(sound.requests.size()==2);
    output.done=true; behavior.update(++now); // LookUp -> Smile stays within one reaction.
    assert(sound.requests.back().effect==SoundEffect::Hold && sound.requests.size()==3);
    const auto heldRequests=sound.requests.size();
    output.done=true; behavior.update(++now); behavior.update(++now);
    assert(sound.requests.size()==heldRequests && sound.playing);
    behavior.handleEvent(InputEvent::RELEASE); assert(!sound.playing);

    behavior.handleEvent(InputEvent::SHAKE_START);
    assert(sound.requests.back().effect==SoundEffect::Shake1 && sound.requests.back().loop);
    const auto shakingRequests=sound.requests.size();
    behavior.handleEvent(InputEvent::SHAKE_START);
    behavior.handleEvent(InputEvent::TAP); behavior.handleEvent(InputEvent::HOLD);
    behavior.update(++now); assert(sound.requests.size()==shakingRequests);
    behavior.handleEvent(InputEvent::SHAKE_END); assert(!sound.playing);
    behavior.handleEvent(InputEvent::TAP); assert(sound.requests.size()==shakingRequests);
    finish();

    behavior.requestState(RobotState::SLEEP);
    assert(sound.requests.back().effect==SoundEffect::FallAsleep); finish();
    behavior.requestState(RobotState::IDLE);
    assert(sound.requests.back().effect==SoundEffect::WakeUp); finish();
    const auto beforeTired=sound.requests.size();
    behavior.requestState(RobotState::TIRED);
    assert(sound.requests.size()==beforeTired && !sound.playing); finish();
    behavior.handleEvent(InputEvent::TAP);
    const auto quickWakeRequests=sound.requests.size();
    output.done=true; behavior.update(++now); // Quick wake -> Angry, no second sound.
    assert(sound.requests.size()==quickWakeRequests && sound.requests.back().effect==SoundEffect::Tap1);
    behavior.preview(AnimationId::LookLeft); assert(!sound.playing); finish();

    behavior.update(++now,{10,21*3600+1800,true});
    assert(sound.requests.back().effect==SoundEffect::FallAsleep);
    const auto scheduleRequests=sound.requests.size();
    behavior.handleEvent(InputEvent::TAP); behavior.handleEvent(InputEvent::SHAKE_START);
    assert(sound.requests.size()==scheduleRequests);
    behavior.setState(RobotState::ERROR); assert(!sound.playing);
    behavior.handleEvent(InputEvent::HOLD); behavior.update(++now);
    assert(sound.requests.size()==scheduleRequests);

    Output bootOutput; FakeSoundOutput bootSound;
    BehaviourProgram boot(bootOutput,bootSound);
    boot.begin(0,{10,23*3600,true});
    assert(boot.state()==RobotState::SLEEP && bootSound.requests.empty());

    for (const auto origin : {RobotState::IDLE,RobotState::TIRED,RobotState::SLEEP}) {
        Output face; FakeSoundOutput audio;
        BehaviourProgram held(face,audio);
        held.setState(origin); held.begin(); held.enableSpontaneous(false);
        audio.requests.clear();
        held.handleEvent(InputEvent::HOLD);
        held.update(1);
        assert(audio.requests.empty() && !audio.playing);
        face.done=true; held.update(2);
        assert(face.last()==AnimationId::Smile);
        assert(audio.requests.size()==1 && audio.requests.back().effect==SoundEffect::Hold);
        face.done=true; held.update(3); held.update(4);
        assert(audio.requests.size()==1); // No replay while the smile is held.
        held.handleEvent(InputEvent::RELEASE); assert(!audio.playing);

        Output earlyFace; FakeSoundOutput earlyAudio;
        BehaviourProgram early(earlyFace,earlyAudio);
        early.setState(origin); early.begin(); early.enableSpontaneous(false);
        earlyAudio.requests.clear();
        early.handleEvent(InputEvent::HOLD);
        early.handleEvent(InputEvent::RELEASE);
        for (unsigned t=1;t<8;++t) { earlyFace.done=true; early.update(t); }
        assert(earlyAudio.requests.empty() && !earlyAudio.playing);

        // Replacing the upward gaze with TAP must never emit a delayed Hold.
        early.handleEvent(InputEvent::HOLD);
        early.handleEvent(InputEvent::TAP);
        for (unsigned t=8;t<16;++t) { earlyFace.done=true; early.update(t); }
        assert(earlyAudio.requests.size()==1 && earlyAudio.requests.back().effect==SoundEffect::Tap1);
    }
    std::cout<<"PASS: sound behavior triggers once, silent actions, held reactions, priorities, schedule and ERROR cancellation\n";
}
