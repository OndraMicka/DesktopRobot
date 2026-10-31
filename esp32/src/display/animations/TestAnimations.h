#pragma once

#include "../Animation.h"

#include <array>
#include <cstddef>

namespace desktoprobot
{
    enum class RobotState
    {
        Idle,
        Tired,
        Sleep
    };

    enum class TestAnimationId
    {
        Blink,
        LookLeft,
        LookRight,
        LookUp,
        LookAround,
        Thinking,
        IdleYawn,
        Shaking,
        Dizzy,
        Angry,
        Smile,
        Bored,
        Listening,
        RandomCuriosity,
        TiredYawn,
        SlowBlink,
        SlowEyeMovement,
        SleepEyeOpen,
        SleepZ,
        WakeUp,
        TiredToSleep,
        IdleToTired,
        IdleToSleep,
        TiredToIdle,
        SleepToTired,
        Count
    };

    struct AnimationTestDefinition
    {
        TestAnimationId id;
        RobotState state;
        const char* label;
        bool transition = false;
        RobotState targetState = RobotState::Idle;
    };

    const std::array<AnimationTestDefinition, static_cast<std::size_t>(TestAnimationId::Count)>& getAnimationTestDefinitions();
    const Animation& getTestAnimation(TestAnimationId id);
    // Prefix ending at the established gaze/smile pose, before its neutral return.
    constexpr std::size_t getExpressionFrameCount(TestAnimationId id) {
        return id == TestAnimationId::LookUp ? 9 : id == TestAnimationId::Smile ? 25 : id == TestAnimationId::Angry ? 17 : 0;
    }
    const Animation& getShakeRecoveryAnimation(const Frame& releaseFrame);
    const Animation& getTouchReturnAnimation(const Frame& releaseFrame);
    const AnimationTestDefinition* getStateTransition(RobotState from, RobotState to);
    const Animation& getStateAnimation(RobotState state);
    const char* getStateName(RobotState state);
}
