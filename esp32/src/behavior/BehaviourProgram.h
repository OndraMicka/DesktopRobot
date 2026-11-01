#pragma once
#include "../display/animations/TestAnimations.h"
#include "../sound/SoundOutput.h"
#include <array>
#include <cstdint>

using AnimationId = desktoprobot::TestAnimationId;
enum class RobotState { IDLE, TIRED, SLEEP, ERROR };
enum class InputEvent { NONE, TAP, HOLD, RELEASE, SHAKE_START, SHAKE_END };

struct LocalTime {
    int64_t day = 0;
    uint32_t second = 0;
    bool valid = false;
};

// A small output port keeps the same behavior independent of Arduino and SDL.
class AnimationOutput {
public:
    virtual ~AnimationOutput() = default;
    virtual void playAnimation(AnimationId id, uint32_t now, bool holdExpression = false) = 0;
    virtual void playTouchWake(RobotState source,uint32_t now) = 0;
    virtual void playTouchLookUp(RobotState source, uint32_t now) = 0;
    virtual void playBase(RobotState state, uint32_t now) = 0;
    virtual void playShakeRecovery(uint32_t now) = 0;
    virtual void playTouchReturn(uint32_t now) = 0;
    virtual bool isAnimationFinished() const = 0;
    virtual void showErrorFace() = 0;
};

struct Activity {
    AnimationId id;
    RobotState state;
    uint16_t weight;
    uint32_t cooldownMs;
};

struct BehaviourConfig {
    uint32_t activityMinMs[3] = {6000, 8000, 12000};
    uint32_t activityMaxMs[3] = {14000, 18000, 25000};
    uint32_t blinkMinMs[2] = {2000, 4000};
    uint32_t blinkMaxMs[2] = {5000, 9000};
    uint32_t spontaneousGapMinMs = 800;
    uint32_t spontaneousGapMaxMs = 2200;
    uint32_t temporaryMs = 300000;
    uint16_t quietWeight = 3;
    uint32_t randomSeed = 0xA57B19U;
    static constexpr unsigned ActivityCount = 23;
    std::array<Activity, ActivityCount> activities{{
        {AnimationId::LookAround, RobotState::IDLE, 35, 8000},
        {AnimationId::Thinking, RobotState::IDLE, 10, 10000},
        {AnimationId::IdleYawn, RobotState::IDLE, 8, 15000},
        {AnimationId::Smile, RobotState::IDLE, 10, 20000},
        {AnimationId::RandomCuriosity, RobotState::IDLE, 15, 15000},
        {AnimationId::Bored, RobotState::IDLE, 8, 20000},
        {AnimationId::Listening, RobotState::IDLE, 7, 20000},
        {AnimationId::LookLeft, RobotState::IDLE, 5, 5000},
        {AnimationId::LookRight, RobotState::IDLE, 5, 5000},
        {AnimationId::LookUp, RobotState::IDLE, 5, 5000},
        {AnimationId::TiredYawn, RobotState::TIRED, 35, 12000},
        {AnimationId::SlowEyeMovement, RobotState::TIRED, 20, 12000},
        {AnimationId::SleepZ, RobotState::SLEEP, 10, 10000},
        {AnimationId::Microsleep, RobotState::TIRED, 18, 20000},
        {AnimationId::Hmm, RobotState::IDLE, 12, 15000},
        {AnimationId::Kiss, RobotState::IDLE, 5, 45000},
        {AnimationId::Sigh, RobotState::TIRED, 20, 15000},
        {AnimationId::SleepRoll, RobotState::SLEEP, 12, 25000},
        {AnimationId::StartleLeft, RobotState::IDLE, 7, 30000},
        {AnimationId::StartleUpRight, RobotState::IDLE, 7, 30000},
        {AnimationId::HeavyLids, RobotState::TIRED, 28, 12000},
        {AnimationId::SleepyNod, RobotState::TIRED, 22, 15000},
        {AnimationId::SleepTwitch, RobotState::SLEEP, 10, 18000}
    }};
    uint16_t sleepPeekWeight = 5;
    uint32_t sleepPeekCooldownMs = 20000;
    uint32_t startleGroupCooldownMs = 20000;
};

class BehaviourProgram {
public:
    using Random = uint32_t (*)(void*);
    explicit BehaviourProgram(AnimationOutput& output, BehaviourConfig config = {}, Random random = nullptr, void* context = nullptr);
    BehaviourProgram(AnimationOutput& output, desktoprobot::SoundOutput& sound,
                     BehaviourConfig config = {}, Random random = nullptr, void* context = nullptr);
    void begin(uint32_t now = 0, LocalTime local = {});
    void handleEvent(InputEvent event);
    void update(uint32_t now, LocalTime local = {});
    void setState(RobotState state);
    void preview(AnimationId id);
    void requestState(RobotState target, bool morning = false);
    void changeTime(uint32_t now, LocalTime local, bool restoreSchedule = false);
    const char* modeName() const;
    const char* temporaryName() const;
    bool touchHeld() const { return held_; }
    uint32_t temporaryRemainingMs() const;

    void enableSpontaneous(bool enabled) { spontaneous_ = enabled; }
    void setTouchPending(bool pending) { touchPending_ = pending; }
    RobotState state() const { return state_; }
    bool busy() const { return mode_ != Mode::Base; }
private:
    enum class Mode { Base, Activity, Blink, Reaction, Transition, Schedule, Shake, Dizzy, TouchReturn };
    enum class Temporary { None, IdleThenTired, TiredThenSleep, Early };
    AnimationOutput& output_;
    desktoprobot::SoundOutput* sound_ = nullptr;
    bool soundsEnabled_ = true;
    BehaviourConfig config_;
    Random random_;
    void* randomContext_;
    uint32_t rng_ = 0xA57B19U, now_ = 0;
    LocalTime local_{};
    RobotState state_ = RobotState::IDLE, returnState_ = RobotState::IDLE;
    RobotState reactionSource_ = RobotState::IDLE;
    Mode mode_ = Mode::Base;
    Temporary temporary_ = Temporary::None;
    uint32_t temporaryStart_ = 0, activityStart_ = 0, activityDelay_ = 0, blinkStart_ = 0, blinkDelay_ = 0;
    uint32_t spontaneousStart_ = 0, spontaneousDelay_ = 0;
    std::array<uint32_t, BehaviourConfig::ActivityCount + 1> lastPlayed_{};
    std::array<bool, BehaviourConfig::ActivityCount + 1> played_{};
    uint32_t lastStartle_ = 0;
    bool startlePlayed_ = false;
    int lastActivity_ = -1;
    int64_t scheduleBoundary_ = 0;
    bool scheduleKnown_ = false, held_ = false, spontaneous_ = true, touchPending_ = false;
    std::array<AnimationId, 4> sequence_{};
    unsigned sequenceCount_ = 0, sequenceIndex_ = 0;
    bool holdLast_ = false, quickWake_ = false;
    void playStep();
    void stopSound();
    void playSound(desktoprobot::SoundEffect effect, bool loop = false);
    uint32_t randomValue();
    uint32_t range(uint32_t min, uint32_t max);
    void resetTimers(bool resetBlink = true);
    void base(bool resetBlink = true);
    void start(Mode mode, const AnimationId* ids, unsigned count, bool holdLast = false);
    void transition(RobotState target, Mode mode, bool morning = false);
    void checkSchedule();
    void finishReaction();
    void chooseActivity();
};
