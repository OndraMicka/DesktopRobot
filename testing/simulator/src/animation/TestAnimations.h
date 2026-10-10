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
        Surprised,
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
        Count
    };

    struct AnimationTestDefinition
    {
        TestAnimationId id;
        RobotState state;
        const char* label;
    };

    const std::array<AnimationTestDefinition, static_cast<std::size_t>(TestAnimationId::Count)>& getAnimationTestDefinitions();
    const Animation& getTestAnimation(TestAnimationId id);
    const Animation& getStateAnimation(RobotState state);
    const char* getStateName(RobotState state);
}
