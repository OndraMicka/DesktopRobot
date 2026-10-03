#include "TestAnimations.h"

#include "animations/blink.h"
#include "animations/idle.h"
#include "animations/sleep.h"

#include <array>
#include <cstdint>

namespace desktoprobot
{
    namespace
    {
        constexpr std::size_t MAX_FRAMES = 3;
        constexpr std::size_t MAX_COMMANDS = 16;

        enum class Mouth
        {
            Flat,
            Smile,
            Frown,
            Open
        };

        struct Pose
        {
            int eyeOffsetX;
            int eyeOffsetY;
            int eyeHeight;
            Mouth mouth;
            bool angryBrows;
            int zOffset;
        };

        struct AnimationStorage
        {
            std::array<std::array<DrawCommand, MAX_COMMANDS>, MAX_FRAMES> commands{};
            std::array<Frame, MAX_FRAMES> frames{};
            Animation animation{};
        };

        constexpr Pose center{ 0, 0, 12, Mouth::Flat, false, -1 };
        constexpr Pose lookLeft{ -7, 0, 12, Mouth::Flat, false, -1 };
        constexpr Pose lookRight{ 7, 0, 12, Mouth::Flat, false, -1 };
        constexpr Pose lookUp{ 0, -5, 12, Mouth::Flat, false, -1 };
        constexpr Pose halfEyes{ 0, 2, 7, Mouth::Flat, false, -1 };
        constexpr Pose closedEyes{ 0, 6, 2, Mouth::Flat, false, -1 };
        constexpr Pose smile{ 0, 0, 12, Mouth::Smile, false, -1 };
        constexpr Pose sleepySmile{ 0, 3, 6, Mouth::Smile, false, -1 };
        constexpr Pose openMouth{ 0, 0, 12, Mouth::Open, false, -1 };
        constexpr Pose angry{ 0, 0, 12, Mouth::Frown, true, -1 };

        void append(std::array<DrawCommand, MAX_COMMANDS>& commands, std::size_t& count, DrawCommand command)
        {
            if (count < commands.size())
            {
                commands[count++] = command;
            }
        }

        void drawPose(std::array<DrawCommand, MAX_COMMANDS>& commands, std::size_t& count, const Pose& pose)
        {
            const int eyeY = 18 + pose.eyeOffsetY;
            for (const int eyeX : { 25 + pose.eyeOffsetX, 83 + pose.eyeOffsetX })
            {
                if (pose.eyeHeight <= 2)
                {
                    append(commands, count, line(eyeX, eyeY + pose.eyeHeight, eyeX + 20, eyeY + pose.eyeHeight));
                }
                else
                {
                    append(commands, count, rectangle(eyeX, eyeY, 20, pose.eyeHeight));
                }
            }

            if (pose.angryBrows)
            {
                append(commands, count, line(25, 15, 45, 20));
                append(commands, count, line(83, 20, 103, 15));
            }

            switch (pose.mouth)
            {
            case Mouth::Flat:
                append(commands, count, rectangle(45, 47, 38, 2));
                break;
            case Mouth::Smile:
                append(commands, count, line(45, 44, 53, 50));
                append(commands, count, line(53, 50, 64, 52));
                append(commands, count, line(64, 52, 75, 50));
                append(commands, count, line(75, 50, 83, 44));
                break;
            case Mouth::Frown:
                append(commands, count, line(45, 51, 53, 46));
                append(commands, count, line(53, 46, 64, 44));
                append(commands, count, line(64, 44, 75, 46));
                append(commands, count, line(75, 46, 83, 51));
                break;
            case Mouth::Open:
                append(commands, count, circle(64, 49, 5));
                break;
            }

            if (pose.zOffset >= 0)
            {
                append(commands, count, line(94, pose.zOffset, 102, pose.zOffset));
                append(commands, count, line(102, pose.zOffset, 94, pose.zOffset + 8));
                append(commands, count, line(94, pose.zOffset + 8, 102, pose.zOffset + 8));
            }
        }

        AnimationStorage createAnimation(
            const char* name,
            const std::array<Pose, MAX_FRAMES>& poses,
            const std::array<uint32_t, MAX_FRAMES>& durations,
            std::size_t frameCount = MAX_FRAMES,
            bool loop = false
        )
        {
            AnimationStorage result{};
            for (std::size_t index = 0; index < frameCount; ++index)
            {
                std::size_t commandCount = 0;
                drawPose(result.commands[index], commandCount, poses[index]);
                result.frames[index] = {
                    durations[index],
                    result.commands[index].data(),
                    commandCount
                };
            }
            result.animation = { name, result.frames.data(), frameCount, loop };
            return result;
        }

        void rebindAnimationStorage(AnimationStorage& storage)
        {
            for (std::size_t index = 0; index < storage.animation.frameCount; ++index)
            {
                storage.frames[index].commands = storage.commands[index].data();
            }
            storage.animation.frames = storage.frames.data();
        }

        void initializeAnimations(
            std::array<AnimationStorage, static_cast<std::size_t>(TestAnimationId::Count)>& result
        )
        {
            const auto add = [&result](
                TestAnimationId id,
                const char* name,
                const std::array<Pose, MAX_FRAMES>& poses,
                const std::array<uint32_t, MAX_FRAMES>& durations,
                std::size_t frameCount = MAX_FRAMES)
            {
                result[static_cast<std::size_t>(id)] = createAnimation(name, poses, durations, frameCount);
                rebindAnimationStorage(result[static_cast<std::size_t>(id)]);
            };

            add(TestAnimationId::LookLeft, "LOOK LEFT", { lookLeft, lookLeft, center }, { 180, 300, 180 });
            add(TestAnimationId::LookRight, "LOOK RIGHT", { lookRight, lookRight, center }, { 180, 300, 180 });
            add(TestAnimationId::LookUp, "LOOK UP", { center, lookUp, lookUp }, { 160, 450, 180 });
            add(TestAnimationId::LookAround, "LOOK AROUND", { lookLeft, lookRight, center }, { 220, 220, 180 });
            add(TestAnimationId::Thinking, "THINKING", { lookUp, lookUp, halfEyes }, { 260, 650, 260 });
            add(TestAnimationId::IdleYawn, "YAWN", { halfEyes, openMouth, center }, { 220, 520, 320 });
            add(TestAnimationId::Surprised, "SURPRISED", {
                Pose{ 0, -2, 19, Mouth::Open, false, -1 }, openMouth, lookLeft
            }, { 180, 380, 260 });
            add(TestAnimationId::Angry, "ANGRY", { angry, angry, center }, { 240, 620, 220 });
            add(TestAnimationId::Content, "CONTENT", { closedEyes, sleepySmile, closedEyes }, { 300, 550, 300 });
            add(TestAnimationId::Bored, "BORED", {
                Pose{ 6, 2, 7, Mouth::Frown, false, -1 },
                Pose{ 6, 2, 7, Mouth::Frown, false, -1 },
                center
            }, { 220, 700, 220 });
            add(TestAnimationId::SelfSatisfied, "SELF SATISFIED", { sleepySmile, smile, sleepySmile }, { 240, 700, 260 });
            add(TestAnimationId::IdleAlmostAsleep, "ALMOST ASLEEP", { halfEyes, closedEyes, Pose{ 0, 0, 14, Mouth::Flat, false, -1 } }, { 400, 700, 160 });
            add(TestAnimationId::Listening, "LISTENING", { center, center, lookRight }, { 180, 650, 300 });
            add(TestAnimationId::RandomCuriosity, "RANDOM CURIOSITY", { lookLeft, lookRight, center }, { 120, 150, 180 });
            add(TestAnimationId::CheckSurroundings, "CHECK SURROUNDINGS", {
                lookLeft, lookUp, lookRight
            }, { 160, 180, 220 });
            add(TestAnimationId::BriefFreeze, "BRIEF FREEZE", { center, center, center }, { 180, 1400, 180 });
            add(TestAnimationId::Hum, "HUM", { smile, openMouth, smile }, { 300, 380, 300 });
            add(TestAnimationId::Smile, "SMILE", { smile, smile, center }, { 220, 700, 220 });
            add(TestAnimationId::TiredYawn, "YAWN", { halfEyes, openMouth, sleepySmile }, { 300, 650, 440 });
            add(TestAnimationId::SlowBlink, "SLOW BLINK", { center, halfEyes, closedEyes }, { 300, 500, 700 });
            add(TestAnimationId::HalfClosedEyes, "HALF CLOSED EYES", { halfEyes, halfEyes, halfEyes }, { 350, 750, 350 });
            add(TestAnimationId::SlowEyeMovement, "SLOW EYE MOVEMENT", { lookLeft, lookRight, center }, { 650, 650, 500 });
            add(TestAnimationId::TiredAlmostAsleep, "ALMOST ASLEEP", { halfEyes, closedEyes, halfEyes }, { 500, 1000, 300 });
            add(TestAnimationId::SleepEyeOpen, "SLEEP EYE OPEN", { closedEyes, halfEyes, closedEyes }, { 300, 700, 450 });
            add(TestAnimationId::SleepZ, "ZZZ", {
                Pose{ 0, 6, 2, Mouth::Flat, false, 12 },
                Pose{ 0, 6, 2, Mouth::Flat, false, 6 },
                Pose{ 0, 6, 2, Mouth::Flat, false, -1 }
            }, { 450, 550, 350 });
            add(TestAnimationId::WakeUp, "WAKE UP", { closedEyes, halfEyes, lookUp }, { 550, 500, 450 });
        }

        const std::array<AnimationTestDefinition, 27> animationTests = {{
            { TestAnimationId::Blink, RobotState::Idle, "BLINK", SoundEffect::Happy, false },
            { TestAnimationId::LookLeft, RobotState::Idle, "LOOK LEFT", SoundEffect::Happy, false },
            { TestAnimationId::LookRight, RobotState::Idle, "LOOK RIGHT", SoundEffect::Happy, false },
            { TestAnimationId::LookUp, RobotState::Idle, "LOOK UP", SoundEffect::Happy, false },
            { TestAnimationId::LookAround, RobotState::Idle, "LOOK AROUND", SoundEffect::Happy, false },
            { TestAnimationId::Thinking, RobotState::Idle, "THINKING", SoundEffect::Happy, false },
            { TestAnimationId::IdleYawn, RobotState::Idle, "YAWN", SoundEffect::Happy, false },
            { TestAnimationId::Surprised, RobotState::Idle, "SURPRISED", SoundEffect::Surprised, true },
            { TestAnimationId::Angry, RobotState::Idle, "ANGRY", SoundEffect::Angry, true },
            { TestAnimationId::Content, RobotState::Idle, "CONTENTED", SoundEffect::Content, true },
            { TestAnimationId::Bored, RobotState::Idle, "BORED", SoundEffect::Bored, true },
            { TestAnimationId::SelfSatisfied, RobotState::Idle, "SELF SATISFIED", SoundEffect::Content, true },
            { TestAnimationId::IdleAlmostAsleep, RobotState::Idle, "ALMOST ASLEEP", SoundEffect::Happy, false },
            { TestAnimationId::Listening, RobotState::Idle, "LISTENING", SoundEffect::Happy, false },
            { TestAnimationId::RandomCuriosity, RobotState::Idle, "RANDOM CURIOSITY", SoundEffect::Happy, false },
            { TestAnimationId::CheckSurroundings, RobotState::Idle, "CHECK SURROUNDINGS", SoundEffect::Happy, false },
            { TestAnimationId::BriefFreeze, RobotState::Idle, "BRIEF FREEZE", SoundEffect::Happy, false },
            { TestAnimationId::Hum, RobotState::Idle, "HUM", SoundEffect::Hum, true },
            { TestAnimationId::Smile, RobotState::Idle, "SMILE", SoundEffect::Happy, true },
            { TestAnimationId::TiredYawn, RobotState::Tired, "YAWN", SoundEffect::Happy, false },
            { TestAnimationId::SlowBlink, RobotState::Tired, "SLOW BLINK", SoundEffect::Happy, false },
            { TestAnimationId::HalfClosedEyes, RobotState::Tired, "HALF CLOSED EYES", SoundEffect::Happy, false },
            { TestAnimationId::SlowEyeMovement, RobotState::Tired, "SLOW EYE MOVEMENT", SoundEffect::Happy, false },
            { TestAnimationId::TiredAlmostAsleep, RobotState::Tired, "ALMOST ASLEEP", SoundEffect::Happy, false },
            { TestAnimationId::SleepEyeOpen, RobotState::Sleep, "SLEEP EYE OPEN", SoundEffect::Surprised, true },
            { TestAnimationId::SleepZ, RobotState::Sleep, "ZZZ", SoundEffect::Happy, false },
            { TestAnimationId::WakeUp, RobotState::Sleep, "WAKE UP", SoundEffect::WakeUp, true }
        }};

        const AnimationStorage& getAnimationStorage(TestAnimationId id)
        {
            static std::array<AnimationStorage, static_cast<std::size_t>(TestAnimationId::Count)> animations{};
            static const bool initialized = []()
            {
                initializeAnimations(animations);
                return true;
            }();
            (void)initialized;
            return animations[static_cast<std::size_t>(id)];
        }

        const Animation& getTiredStateAnimation()
        {
            static AnimationStorage animation{};
            static const bool initialized = []()
            {
                animation = createAnimation(
                    "TIRED",
                    { halfEyes, sleepySmile, halfEyes },
                    { 800, 800, 800 },
                    3,
                    true
                );
                rebindAnimationStorage(animation);
                return true;
            }();
            (void)initialized;
            return animation.animation;
        }
    }

    const std::array<AnimationTestDefinition, 27>& getAnimationTestDefinitions()
    {
        return animationTests;
    }

    const Animation& getTestAnimation(TestAnimationId id)
    {
        if (id == TestAnimationId::Blink)
        {
            return blinkAnimation;
        }
        return getAnimationStorage(id).animation;
    }

    const Animation& getStateAnimation(RobotState state)
    {
        switch (state)
        {
        case RobotState::Idle:
            return idleAnimation;
        case RobotState::Tired:
            return getTiredStateAnimation();
        case RobotState::Sleep:
            return sleepAnimation;
        }
        return idleAnimation;
    }

    const char* getStateName(RobotState state)
    {
        switch (state)
        {
        case RobotState::Idle:
            return "IDLE";
        case RobotState::Tired:
            return "TIRED";
        case RobotState::Sleep:
            return "SLEEP";
        }
        return "IDLE";
    }
}
