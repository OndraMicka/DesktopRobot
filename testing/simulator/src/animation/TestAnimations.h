#pragma once

#include "Animation.h"

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
    const Animation& getShakeRecoveryAnimation(const Frame& releaseFrame);
    const AnimationTestDefinition* getStateTransition(RobotState from, RobotState to);
    const Animation& getStateAnimation(RobotState state);
    const char* getStateName(RobotState state);
}
