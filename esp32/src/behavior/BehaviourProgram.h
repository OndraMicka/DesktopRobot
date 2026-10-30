#pragma once
#include "../display/animations/TestAnimations.h"
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
    virtual void playBase(RobotState state, uint32_t now) = 0;
    virtual void playShakeRecovery(uint32_t now) = 0;
    virtual bool isAnimationFinished() const = 0;
    virtual void showErrorFace() = 0;
    virtual void interactionSound(bool /*friendly*/) {}
};

struct Activity {
    AnimationId id;
    RobotState state;
    uint16_t weight;
    uint32_t cooldownMs;
};

struct BehaviourConfig {
    uint32_t activityMinMs[3] = {10000, 20000, 30000};
    uint32_t activityMaxMs[3] = {60000, 90000, 120000};
    uint32_t blinkMinMs[2] = {3000, 6000};
    uint32_t blinkMaxMs[2] = {7000, 14000};
    uint32_t temporaryMs = 300000;
    uint16_t quietWeight = 15;
    uint32_t randomSeed = 0xA57B19U;
    std::array<Activity, 13> activities{{
        {AnimationId::LookAround, RobotState::IDLE, 35, 20000},
        {AnimationId::Thinking, RobotState::IDLE, 10, 60000},
        {AnimationId::IdleYawn, RobotState::IDLE, 8, 180000},
        {AnimationId::Smile, RobotState::IDLE, 10, 90000},
        {AnimationId::RandomCuriosity, RobotState::IDLE, 5, 45000},
        {AnimationId::Bored, RobotState::IDLE, 5, 90000},
        {AnimationId::Listening, RobotState::IDLE, 7, 60000},
        {AnimationId::LookLeft, RobotState::IDLE, 5, 15000},
        {AnimationId::LookRight, RobotState::IDLE, 5, 15000},
        {AnimationId::LookUp, RobotState::IDLE, 5, 15000},
        {AnimationId::TiredYawn, RobotState::TIRED, 35, 45000},
        {AnimationId::SlowEyeMovement, RobotState::TIRED, 20, 30000},
        {AnimationId::SleepZ, RobotState::SLEEP, 35, 40000}
    }};
    uint16_t sleepPeekWeight = 5;
    uint32_t sleepPeekCooldownMs = 120000;
};

class BehaviourProgram {
public:
    using Random = uint32_t (*)(void*);
    explicit BehaviourProgram(AnimationOutput& output, BehaviourConfig config = {}, Random random = nullptr, void* context = nullptr);
    void begin(uint32_t now = 0, LocalTime local = {});
    void handleEvent(InputEvent event);
    void update(uint32_t now, LocalTime local = {});
    void setState(RobotState state);
    void preview(AnimationId id);
    void enableSpontaneous(bool enabled) { spontaneous_ = enabled; }
    RobotState state() const { return state_; }
    bool busy() const { return mode_ != Mode::Base; }
private:
    enum class Mode { Base, Activity, Blink, Reaction, Transition, Schedule, Shake, Dizzy };
    enum class Temporary { None, IdleThenTired, TiredThenSleep, Early };
    AnimationOutput& output_;
    BehaviourConfig config_;
    Random random_;
    void* randomContext_;
    uint32_t rng_ = 0xA57B19U, now_ = 0;
    LocalTime local_{};
    RobotState state_ = RobotState::IDLE, returnState_ = RobotState::IDLE;
    Mode mode_ = Mode::Base;
    Temporary temporary_ = Temporary::None;
    uint32_t temporaryStart_ = 0, activityStart_ = 0, activityDelay_ = 0, blinkStart_ = 0, blinkDelay_ = 0;
    std::array<uint32_t, 14> lastPlayed_{};
    std::array<bool, 14> played_{};
    int lastActivity_ = -1;
    int64_t scheduleBoundary_ = 0;
    bool scheduleKnown_ = false, held_ = false, spontaneous_ = true;
    std::array<AnimationId, 4> sequence_{};
    unsigned sequenceCount_ = 0, sequenceIndex_ = 0;
    bool holdLast_ = false;
    uint32_t randomValue();
    uint32_t range(uint32_t min, uint32_t max);
    void resetTimers();
    void base();
    void start(Mode mode, const AnimationId* ids, unsigned count, bool holdLast = false);
    void transition(RobotState target, Mode mode, bool morning = false);
    void checkSchedule();
    void finishReaction();
    void chooseActivity();
};
