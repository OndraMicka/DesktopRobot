#pragma once

#include "Animation.h"

#include "../sound/SoundPlayer.h"

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
        Content,
        Bored,
        SelfSatisfied,
        IdleAlmostAsleep,
        Listening,
        RandomCuriosity,
        CheckSurroundings,
        BriefFreeze,
        Hum,
        Smile,
        TiredYawn,
        SlowBlink,
        HalfClosedEyes,
        SlowEyeMovement,
        TiredAlmostAsleep,
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
        SoundEffect sound;
        bool playsSound;
    };

    const std::array<AnimationTestDefinition, 27>& getAnimationTestDefinitions();
    const Animation& getTestAnimation(TestAnimationId id);
    const Animation& getStateAnimation(RobotState state);
    const char* getStateName(RobotState state);
}
