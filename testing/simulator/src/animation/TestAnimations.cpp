#include "TestAnimations.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace desktoprobot
{
    namespace
    {
        constexpr std::size_t MAX_FRAMES = 11;
        constexpr std::size_t MAX_COMMANDS = 20;

        enum class Mouth : uint8_t
        {
            Flat,
            Smile,
            Frown,
            Open,
            Sleep
        };

        enum class Brow : uint8_t
        {
            Neutral,
            Angry,
            Worried,
            Curious
        };

        struct Pose
        {
            int eyeOffsetX;
            int eyeOffsetY;
            int eyeWidth;
            int eyeHeight;
            int gazeX;
            int gazeY;
            int eyeTilt;
            int rightEyeHeightOffset;
            Mouth mouth;
            Brow brow;
            int zOffset;
        };

        struct AnimationStorage
        {
            std::array<std::array<DrawCommand, MAX_COMMANDS>, MAX_FRAMES> commands{};
            std::array<Frame, MAX_FRAMES> frames{};
            Animation animation{};
        };

        constexpr Pose neutral{ 0, 0, 36, 27, 0, 0, 0, 0, Mouth::Flat, Brow::Neutral, -1 };
        constexpr Pose glanceLeft{ 0, 0, 36, 27, -7, 0, 0, 0, Mouth::Flat, Brow::Neutral, -1 };
        constexpr Pose glanceRight{ 0, 0, 36, 27, 7, 0, 0, 0, Mouth::Flat, Brow::Neutral, -1 };
        constexpr Pose glanceUp{ 0, -2, 36, 27, 0, -6, 0, 0, Mouth::Flat, Brow::Curious, -1 };
        constexpr Pose halfLid{ 0, 5, 36, 18, 0, 0, 0, 0, Mouth::Flat, Brow::Neutral, -1 };
        constexpr Pose tired{ 0, 7, 34, 14, 0, 1, 0, -2, Mouth::Flat, Brow::Worried, -1 };
        constexpr Pose eyesClosed{ 0, 11, 36, 5, 0, 0, 0, 0, Mouth::Sleep, Brow::Neutral, -1 };
        constexpr Pose eyesVeryClosed{ 0, 12, 36, 3, 0, 0, 0, 0, Mouth::Sleep, Brow::Neutral, -1 };
        constexpr Pose happy{ 0, 1, 36, 23, 0, 2, -1, -1, Mouth::Smile, Brow::Neutral, -1 };
        constexpr Pose sleepyHappy{ 0, 7, 34, 14, 0, 1, 0, -2, Mouth::Smile, Brow::Worried, -1 };
        constexpr Pose yawn{ 0, 1, 34, 24, 0, 0, 0, 0, Mouth::Open, Brow::Worried, -1 };
        constexpr Pose surprised{ 0, -1, 37, 31, 0, -1, 0, 0, Mouth::Open, Brow::Curious, -1 };
        constexpr Pose angry{ 0, 2, 36, 23, 0, 1, -4, 4, Mouth::Frown, Brow::Angry, -1 };
        constexpr Pose bored{ 0, 7, 35, 16, 5, 0, 2, -1, Mouth::Frown, Brow::Worried, -1 };
        constexpr Pose zSmall{ 0, 11, 36, 5, 0, 0, 0, 0, Mouth::Sleep, Brow::Neutral, 12 };
        constexpr Pose zHigh{ 0, 11, 36, 5, 0, 0, 0, 0, Mouth::Sleep, Brow::Neutral, 1 };

        constexpr int clamp(int value, int low, int high)
        {
            return value < low ? low : (value > high ? high : value);
        }

        int interpolate(int start, int end, int step, int steps)
        {
            const int numerator = start * (steps - step) + end * step;
            return numerator >= 0
                ? (numerator + steps / 2) / steps
                : (numerator - steps / 2) / steps;
        }

        Pose interpolatePose(const Pose& start, const Pose& end, int step, int steps)
        {
            return {
                interpolate(start.eyeOffsetX, end.eyeOffsetX, step, steps),
                interpolate(start.eyeOffsetY, end.eyeOffsetY, step, steps),
                interpolate(start.eyeWidth, end.eyeWidth, step, steps),
                interpolate(start.eyeHeight, end.eyeHeight, step, steps),
                interpolate(start.gazeX, end.gazeX, step, steps),
                interpolate(start.gazeY, end.gazeY, step, steps),
                interpolate(start.eyeTilt, end.eyeTilt, step, steps),
                interpolate(start.rightEyeHeightOffset, end.rightEyeHeightOffset, step, steps),
                step * 2 >= steps ? end.mouth : start.mouth,
                step * 2 >= steps ? end.brow : start.brow,
                start.zOffset < 0 || end.zOffset < 0
                    ? -1
                    : interpolate(start.zOffset, end.zOffset, step, steps)
            };
        }

        void append(std::array<DrawCommand, MAX_COMMANDS>& commands, std::size_t& count, DrawCommand command)
        {
            if (count < commands.size())
            {
                commands[count++] = command;
            }
        }

        void drawEye(
            std::array<DrawCommand, MAX_COMMANDS>& commands,
            std::size_t& count,
            int centerX,
            int centerY,
            int width,
            int height,
            int gazeX,
            int gazeY,
            int tilt
        )
        {
            const int x = centerX - width / 2;
            const int y = centerY - height / 2;
            const int cornerRadius = std::min(height / 2, std::max(2, width / 5));
            append(commands, count, roundedRectangle(x, y, width, height, cornerRadius));

            if (height >= 13)
            {
                const int pupilWidth = std::max(5, width / 5);
                const int pupilHeight = std::min(height - 6, std::max(7, height / 2));
                const int pupilX = x + width / 2 - pupilWidth / 2 + gazeX;
                const int pupilY = y + height / 2 - pupilHeight / 2 + gazeY;
                append(commands, count, roundedRectangle(
                    pupilX,
                    pupilY,
                    pupilWidth,
                    pupilHeight,
                    std::min(pupilWidth / 2, pupilHeight / 3),
                    false
                ));
            }

            if (tilt != 0)
            {
                const int inset = std::min(5, width / 5);
                append(commands, count, line(
                    x + inset,
                    y + height / 3 + tilt,
                    x + width - inset,
                    y + height / 3 - tilt,
                    false
                ));
            }
        }

        void drawMouth(
            std::array<DrawCommand, MAX_COMMANDS>& commands,
            std::size_t& count,
            Mouth mouth,
            Brow brow
        )
        {
            switch (mouth)
            {
            case Mouth::Flat:
                append(commands, count, roundedRectangle(53, 49, 22, 3, 1));
                break;
            case Mouth::Smile:
                append(commands, count, line(49, 46, 56, 51));
                append(commands, count, line(56, 51, 64, 53));
                append(commands, count, line(64, 53, 72, 51));
                append(commands, count, line(72, 51, 79, 46));
                break;
            case Mouth::Frown:
                append(commands, count, line(50, 53, 57, 48));
                append(commands, count, line(57, 48, 64, 47));
                append(commands, count, line(64, 47, 71, 48));
                append(commands, count, line(71, 48, 78, 53));
                break;
            case Mouth::Open:
                append(commands, count, circle(64, 50, 5, true));
                break;
            case Mouth::Sleep:
                append(commands, count, line(57, 50, 71, 50));
                break;
            }

            if (brow == Brow::Angry)
            {
                append(commands, count, line(12, 12, 42, 19));
                append(commands, count, line(86, 19, 116, 12));
            }
            else if (brow == Brow::Worried)
            {
                append(commands, count, line(12, 13, 42, 10));
                append(commands, count, line(86, 10, 116, 13));
            }
            else if (brow == Brow::Curious)
            {
                append(commands, count, line(14, 10, 40, 10));
                append(commands, count, line(88, 9, 114, 12));
            }
        }

        void drawPose(std::array<DrawCommand, MAX_COMMANDS>& commands, std::size_t& count, const Pose& pose)
        {
            const int eyeCenterY = 27 + pose.eyeOffsetY;
            const int leftCenterX = 35 + pose.eyeOffsetX;
            const int rightCenterX = 93 + pose.eyeOffsetX;
            drawEye(
                commands, count, leftCenterX, eyeCenterY,
                pose.eyeWidth, pose.eyeHeight, pose.gazeX, pose.gazeY, pose.eyeTilt
            );
            drawEye(
                commands, count, rightCenterX, eyeCenterY,
                pose.eyeWidth,
                clamp(pose.eyeHeight + pose.rightEyeHeightOffset, 3, 33),
                pose.gazeX,
                pose.gazeY,
                -pose.eyeTilt
            );
            drawMouth(commands, count, pose.mouth, pose.brow);

            if (pose.zOffset >= 0)
            {
                const int x = 98;
                const int y = pose.zOffset;
                append(commands, count, line(x, y, x + 8, y));
                append(commands, count, line(x + 8, y, x, y + 8));
                append(commands, count, line(x, y + 8, x + 8, y + 8));
            }
        }

        AnimationStorage createAnimation(
            const char* name,
            const std::array<Pose, 3>& poses,
            const std::array<uint32_t, 3>& durations,
            std::size_t poseCount = 3,
            bool loop = false
        )
        {
            AnimationStorage result{};
            std::size_t frameCount = 0;

            if (poseCount == 1)
            {
                std::size_t commandCount = 0;
                drawPose(result.commands[frameCount], commandCount, poses[0]);
                result.frames[frameCount++] = {
                    durations[0], result.commands[0].data(), commandCount
                };
            }
            else
            {
                constexpr int MORPH_STEPS = 5;
                for (std::size_t segment = 0; segment + 1 < poseCount; ++segment)
                {
                    for (int step = 0; step < MORPH_STEPS; ++step)
                    {
                        const Pose pose = interpolatePose(
                            poses[segment],
                            poses[segment + 1],
                            step,
                            MORPH_STEPS
                        );
                        std::size_t commandCount = 0;
                        drawPose(result.commands[frameCount], commandCount, pose);
                        result.frames[frameCount] = {
                            std::max(1U, durations[segment] / MORPH_STEPS),
                            result.commands[frameCount].data(),
                            commandCount
                        };
                        ++frameCount;
                    }
                }
                std::size_t commandCount = 0;
                drawPose(result.commands[frameCount], commandCount, poses[poseCount - 1]);
                result.frames[frameCount] = {
                    durations[poseCount - 1],
                    result.commands[frameCount].data(),
                    commandCount
                };
                ++frameCount;
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
                const std::array<Pose, 3>& poses,
                const std::array<uint32_t, 3>& durations,
                std::size_t poseCount = 3)
            {
                auto& animation = result[static_cast<std::size_t>(id)];
                animation = createAnimation(name, poses, durations, poseCount);
                rebindAnimationStorage(animation);
            };

            add(TestAnimationId::Blink, "BLINK", { neutral, eyesClosed, neutral }, { 260, 240, 100 });
            add(TestAnimationId::LookLeft, "LOOK LEFT", { neutral, glanceLeft, neutral }, { 320, 520, 260 });
            add(TestAnimationId::LookRight, "LOOK RIGHT", { neutral, glanceRight, neutral }, { 320, 520, 260 });
            add(TestAnimationId::LookUp, "LOOK UP", { neutral, glanceUp, neutral }, { 320, 620, 280 });
            add(TestAnimationId::LookAround, "LOOK AROUND", { glanceLeft, glanceRight, neutral }, { 360, 360, 260 });
            add(TestAnimationId::Thinking, "THINKING", { neutral, glanceUp, halfLid }, { 340, 750, 300 });
            add(TestAnimationId::IdleYawn, "YAWN", { neutral, yawn, neutral }, { 500, 700, 460 });
            add(TestAnimationId::Surprised, "SURPRISED", { neutral, surprised, glanceLeft }, { 320, 650, 420 });
            add(TestAnimationId::Angry, "ANGRY", { neutral, angry, neutral }, { 300, 700, 420 });
            add(TestAnimationId::Content, "CONTENTED", { neutral, happy, eyesClosed }, { 440, 650, 450 });
            add(TestAnimationId::Bored, "BORED", { neutral, bored, bored }, { 420, 1000, 200 });
            add(TestAnimationId::SelfSatisfied, "SELF SATISFIED", { neutral, happy, sleepyHappy }, { 400, 750, 400 });
            add(TestAnimationId::IdleAlmostAsleep, "ALMOST ASLEEP", { neutral, eyesClosed, surprised }, { 720, 620, 260 });
            add(TestAnimationId::Listening, "LISTENING", { neutral, glanceRight, glanceRight }, { 320, 820, 240 });
            add(TestAnimationId::RandomCuriosity, "RANDOM CURIOSITY", { glanceLeft, glanceRight, neutral }, { 250, 280, 240 });
            add(TestAnimationId::CheckSurroundings, "CHECK SURROUNDINGS", { glanceLeft, glanceUp, glanceRight }, { 280, 300, 320 });
            add(TestAnimationId::BriefFreeze, "BRIEF FREEZE", { neutral, neutral, neutral }, { 320, 1200, 220 });
            add(TestAnimationId::Hum, "HUM", { neutral, happy, happy }, { 360, 620, 320 });
            add(TestAnimationId::Smile, "SMILE", { neutral, happy, neutral }, { 360, 800, 420 });
            add(TestAnimationId::TiredYawn, "YAWN", { tired, yawn, sleepyHappy }, { 480, 760, 520 });
            add(TestAnimationId::SlowBlink, "SLOW BLINK", { tired, eyesClosed, tired }, { 820, 720, 650 });
            add(TestAnimationId::HalfClosedEyes, "HALF CLOSED EYES", { tired, sleepyHappy, tired }, { 500, 700, 460 });
            add(TestAnimationId::SlowEyeMovement, "SLOW EYE MOVEMENT", {
                Pose{ 0, 7, 34, 14, -6, 0, 0, -2, Mouth::Flat, Brow::Worried, -1 },
                Pose{ 0, 7, 34, 14, 6, 0, 0, -2, Mouth::Flat, Brow::Worried, -1 },
                tired
            }, { 900, 900, 700 });
            add(TestAnimationId::TiredAlmostAsleep, "ALMOST ASLEEP", { tired, eyesVeryClosed, tired }, { 900, 850, 380 });
            add(TestAnimationId::SleepEyeOpen, "SLEEP EYE OPEN", { eyesClosed, halfLid, eyesClosed }, { 420, 740, 520 });
            add(TestAnimationId::SleepZ, "ZZZ", { zSmall, zHigh, zSmall }, { 720, 850, 720 });
            add(TestAnimationId::WakeUp, "WAKE UP", { eyesClosed, halfLid, glanceUp }, { 700, 620, 500 });

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

        AnimationStorage createStateAnimation(const char* name, const Pose& pose)
        {
            AnimationStorage result = createAnimation(name, { pose, pose, pose }, { 900, 900, 900 }, 3, true);
            rebindAnimationStorage(result);
            return result;
        }

        const std::array<AnimationStorage, static_cast<std::size_t>(TestAnimationId::Count)>& getAnimations()
        {
            static std::array<AnimationStorage, static_cast<std::size_t>(TestAnimationId::Count)> result{};
            static const bool initialized = []()
            {
                initializeAnimations(result);
                return true;
            }();
            (void)initialized;
            return result;
        }

        const AnimationStorage& getIdleStateAnimation()
        {
            static AnimationStorage result{};
            static const bool initialized = []()
            {
                result = createStateAnimation("IDLE", neutral);
                rebindAnimationStorage(result);
                return true;
            }();
            (void)initialized;
            return result;
        }

        const AnimationStorage& getTiredStateAnimation()
        {
            static AnimationStorage result{};
            static const bool initialized = []()
            {
                result = createStateAnimation("TIRED", tired);
                rebindAnimationStorage(result);
                return true;
            }();
            (void)initialized;
            return result;
        }

        const AnimationStorage& getSleepStateAnimation()
        {
            static AnimationStorage result{};
            static const bool initialized = []()
            {
                result = createStateAnimation("SLEEP", eyesClosed);
                rebindAnimationStorage(result);
                return true;
            }();
            (void)initialized;
            return result;
        }
    }

    const std::array<AnimationTestDefinition, 27>& getAnimationTestDefinitions()
    {
        return animationTests;
    }

    const Animation& getTestAnimation(TestAnimationId id)
    {
        return getAnimations()[static_cast<std::size_t>(id)].animation;
    }

    const Animation& getStateAnimation(RobotState state)
    {
        switch (state)
        {
        case RobotState::Idle:
            return getIdleStateAnimation().animation;
        case RobotState::Tired:
            return getTiredStateAnimation().animation;
        case RobotState::Sleep:
            return getSleepStateAnimation().animation;
        }
        return getIdleStateAnimation().animation;
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
