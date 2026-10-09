// CODEX TEST
 #include "TestAnimations.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace desktoprobot
{
    namespace
    {
        constexpr std::size_t MAX_FRAMES = 33;
        constexpr std::size_t MAX_COMMANDS = 32;

        enum class Mouth : uint8_t
        {
            None,
            Smile,
            Open
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
            int zOffset;
            int eyeSpacing = 60;
            int eyeArc = 0;
        };

        struct AnimationStorage
        {
            std::array<std::array<DrawCommand, MAX_COMMANDS>, MAX_FRAMES> commands{};
            std::array<Frame, MAX_FRAMES> frames{};
            Animation animation{};
        };

        // One OLED character: large solid eyes, fixed spacing and a bold, simple mouth.
        // Gaze moves the whole eye; negative eyeArc makes a happy closed-eye arch.
        constexpr Pose neutral{ 0, 0, 32, 28, 0, 0, 0, 0, Mouth::None, -1 };
        constexpr Pose glanceLeft{ 0, 0, 32, 28, -7, 0, 0, 0, Mouth::None, -1 };
        constexpr Pose glanceRight{ 0, 0, 32, 28, 7, 0, 0, 0, Mouth::None, -1 };
        constexpr Pose glanceUp{ 0, 0, 32, 28, 0, -6, 0, 0, Mouth::None, -1 };
        constexpr Pose thinking{ 0, 0, 32, 16, 5, -4, 0, 0, Mouth::None, -1 };
        constexpr Pose halfLid{ 0, 0, 32, 14, 0, 0, 0, 0, Mouth::None, -1 };
        constexpr Pose tired{ 0, 0, 32, 12, 0, 0, 0, 0, Mouth::None, -1 };
        constexpr Pose almostClosed{ 0, 0, 32, 5, 0, 0, 0, 0, Mouth::None, -1 };
        constexpr Pose eyesClosed{ 0, 0, 32, 3, 0, 0, 0, 0, Mouth::None, -1 };
        constexpr Pose eyesVeryClosed{ 0, 0, 32, 2, 0, 0, 0, 0, Mouth::None, -1 };
        constexpr Pose happy{ 0, 0, 32, 4, 0, 0, 0, 0, Mouth::Smile, -1, 60, -4 };
        constexpr Pose sleepyHappy{ 0, 0, 32, 4, 0, 0, 0, 0, Mouth::Smile, -1, 60, -2 };
        constexpr Pose yawn{ 0, 0, 32, 9, 0, 0, 0, 0, Mouth::Open, -1 };
        constexpr Pose surprised{ 0, 0, 32, 32, 0, 0, 0, 0, Mouth::Open, -1 };
        constexpr Pose angry{ 0, 0, 32, 16, 0, 0, 3, 0, Mouth::None, -1 };
        constexpr Pose bored{ 0, 0, 32, 10, 4, 0, 0, 0, Mouth::None, -1 };
        constexpr Pose zSmall{ 0, 0, 32, 3, 0, 0, 0, 0, Mouth::None, 12 };
        constexpr Pose zHigh{ 0, 0, 32, 3, 0, 0, 0, 0, Mouth::None, 1 };

        constexpr Pose shifted(Pose pose, int x, int y)
        {
            pose.eyeOffsetX += x;
            pose.eyeOffsetY += y;
            return pose;
        }

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
            // Ease in and out instead of moving at a constant, mechanical speed.
            step = step * step * (3 * steps - 2 * step);
            steps = steps * steps * steps;
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
                start.zOffset < 0 || end.zOffset < 0
                    ? -1
                    : interpolate(start.zOffset, end.zOffset, step, steps),
                interpolate(start.eyeSpacing, end.eyeSpacing, step, steps),
                interpolate(start.eyeArc, end.eyeArc, step, steps)
            };
        }

        void append(std::array<DrawCommand, MAX_COMMANDS>& commands, std::size_t& count, DrawCommand command)
        {
            if (count < commands.size())
            {
                commands[count++] = command;
            }
        }

        void drawSoftArc(
            std::array<DrawCommand, MAX_COMMANDS>& commands,
            std::size_t& count,
            int x,
            int y,
            int width,
            int bend,
            int radius
        )
        {
            // Overlapping filled circles form a thick curve with round ends.
            constexpr int STEPS = 8;
            const int span = width - 1 - 2 * radius;
            for (int step = 0; step <= STEPS; ++step)
            {
                const int curveY = interpolate(0, bend, 4 * step * (STEPS - step), STEPS * STEPS);
                append(commands, count, circle(
                    x + radius + interpolate(0, span, step, STEPS), y + curveY, radius, true
                ));
            }
        }

        void drawEye(
            std::array<DrawCommand, MAX_COMMANDS>& commands,
            std::size_t& count,
            int centerX,
            int centerY,
            int width,
            int height,
            int tilt,
            int eyeArc
        )
        {
            const int x = centerX - width / 2;
            const int y = centerY - height / 2;
            if (height <= 5)
            {
                const int bend = eyeArc != 0 ? eyeArc : (height <= 2 ? 0 : 2);
                drawSoftArc(commands, count, x, centerY, width, bend, 2);
                return;
            }

            const int cornerRadius = std::min(width, height) / 2;
            append(commands, count, roundedRectangle(x, y, width, height, cornerRadius));

            if (tilt != 0)
            {
                // Only anger needs a brow: one short inward-sloping stroke per eye.
                const int slope = clamp(tilt, -3, 3);
                for (int row = 0; row < 3; ++row)
                {
                    append(commands, count, line(
                        x, y - 6 - slope + row, x + width - 1, y - 6 + slope + row
                    ));
                }
            }
        }

        void drawMouth(
            std::array<DrawCommand, MAX_COMMANDS>& commands,
            std::size_t& count,
            Mouth mouth,
            int offsetX,
            int offsetY
        )
        {
            const std::size_t firstCommand = count;
            switch (mouth)
            {
            case Mouth::None:
                append(commands, count, roundedRectangle(54, 49, 20, 5, 2));
                break;
            case Mouth::Smile:
                drawSoftArc(commands, count, 49, 47, 31, 6, 3);
                break;
            case Mouth::Open:
                append(commands, count, roundedRectangle(57, 43, 14, 17, 7));
                break;
            }
            for (std::size_t index = firstCommand; index < count; ++index)
            {
                commands[index].x1 += offsetX;
                commands[index].y1 += offsetY;
            }
        }

        void drawPose(std::array<DrawCommand, MAX_COMMANDS>& commands, std::size_t& count, const Pose& pose)
        {
            const int eyeCenterY = 25 + pose.eyeOffsetY + pose.gazeY;
            const int leftCenterX = 64 - pose.eyeSpacing / 2 + pose.eyeOffsetX + pose.gazeX;
            const int rightCenterX = 64 + pose.eyeSpacing / 2 + pose.eyeOffsetX + pose.gazeX;
            drawEye(
                commands, count, leftCenterX, eyeCenterY,
                pose.eyeWidth, pose.eyeHeight, pose.eyeTilt, pose.eyeArc
            );
            drawEye(
                commands, count, rightCenterX, eyeCenterY,
                pose.eyeWidth,
                clamp(pose.eyeHeight + pose.rightEyeHeightOffset, 2, 36),
                -pose.eyeTilt,
                pose.eyeArc
            );
            drawMouth(commands, count, pose.mouth, pose.eyeOffsetX, pose.eyeOffsetY);

            if (pose.zOffset >= 0)
            {
                const int x = 113;
                const int y = pose.zOffset;
                append(commands, count, roundedRectangle(x, y, 11, 3, 1));
                for (int row = 0; row < 3; ++row)
                {
                    append(commands, count, line(x + 10, y + row, x, y + 10 + row));
                }
                append(commands, count, roundedRectangle(x, y + 10, 11, 3, 1));
            }
        }

        AnimationStorage createAnimation(
            const char* name,
            const std::array<Pose, 5>& poses,
            const std::array<uint32_t, 5>& durations,
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
                constexpr int MORPH_STEPS = 8;
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
                        // Pause on the expression, then move through the next gesture.
                        const uint32_t holdMs = durations[segment] / 3;
                        result.frames[frameCount] = {
                            std::max(1U, step == 0 ? holdMs : (durations[segment] - holdMs) / (MORPH_STEPS - 1)),
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
                const std::array<Pose, 5>& poses,
                const std::array<uint32_t, 5>& durations,
                std::size_t poseCount = 3)
            {
                auto& animation = result[static_cast<std::size_t>(id)];
                auto longerDurations = durations;
                for (std::size_t index = 0; index < poseCount; ++index)
                {
                    longerDurations[index] += durations[index] / 2;
                }
                animation = createAnimation(name, poses, longerDurations, poseCount);
                rebindAnimationStorage(animation);
            };

            add(TestAnimationId::Blink, "BLINK", { neutral, eyesClosed, neutral, eyesClosed, neutral }, { 140, 90, 220, 90, 180 }, 5);
            add(TestAnimationId::LookLeft, "LOOK LEFT", { neutral, glanceLeft, shifted(glanceLeft, -2, 1), glanceLeft, neutral }, { 240, 420, 300, 220, 240 }, 5);
            add(TestAnimationId::LookRight, "LOOK RIGHT", { neutral, glanceRight, shifted(glanceRight, 2, 1), glanceRight, neutral }, { 240, 420, 300, 220, 240 }, 5);
            add(TestAnimationId::LookUp, "LOOK UP", { neutral, glanceUp, shifted(glanceUp, 0, 2), glanceUp, neutral }, { 240, 440, 260, 220, 260 }, 5);
            add(TestAnimationId::LookAround, "LOOK AROUND", { neutral, glanceLeft, glanceUp, glanceRight, neutral }, { 220, 420, 320, 420, 240 }, 5);
            add(TestAnimationId::Thinking, "THINKING", { neutral, thinking, shifted(thinking, -2, 2), thinking, neutral }, { 280, 600, 420, 500, 260 }, 5);
            add(TestAnimationId::IdleYawn, "YAWN", { neutral, halfLid, shifted(yawn, 0, -2), sleepyHappy, neutral }, { 260, 360, 720, 440, 280 }, 5);
            add(TestAnimationId::Surprised, "SURPRISED", { neutral, shifted(surprised, 0, -2), surprised, glanceLeft, neutral }, { 140, 320, 540, 360, 220 }, 5);
            add(TestAnimationId::Angry, "ANGRY", { neutral, shifted(angry, -2, 0), shifted(angry, 2, 0), angry, neutral }, { 220, 280, 280, 600, 260 }, 5);
            add(TestAnimationId::Content, "CONTENTED", { neutral, happy, shifted(happy, 0, -2), sleepyHappy, neutral }, { 260, 400, 420, 500, 280 }, 5);
            add(TestAnimationId::Bored, "BORED", { neutral, bored, shifted(bored, 0, 2), bored, neutral }, { 320, 700, 500, 440, 260 }, 5);
            add(TestAnimationId::SelfSatisfied, "SELF SATISFIED", { neutral, happy, shifted(sleepyHappy, 2, -1), sleepyHappy, neutral }, { 260, 480, 420, 520, 280 }, 5);
            add(TestAnimationId::IdleAlmostAsleep, "ALMOST ASLEEP", { neutral, eyesClosed, surprised }, { 720, 620, 260 });
            add(TestAnimationId::Listening, "LISTENING", { neutral, glanceRight, shifted(glanceRight, 0, -2), glanceRight, neutral }, { 260, 500, 400, 500, 260 }, 5);
            add(TestAnimationId::RandomCuriosity, "RANDOM CURIOSITY", { neutral, glanceLeft, glanceUp, glanceRight, neutral }, { 200, 260, 300, 280, 220 }, 5);
            add(TestAnimationId::CheckSurroundings, "CHECK SURROUNDINGS", { neutral, glanceLeft, glanceUp, glanceRight, neutral }, { 240, 480, 420, 480, 260 }, 5);
            add(TestAnimationId::BriefFreeze, "BRIEF FREEZE", { neutral, neutral, neutral }, { 320, 1200, 220 });
            add(TestAnimationId::Hum, "HUM", { neutral, shifted(happy, -2, -1), shifted(happy, 2, 0), happy, neutral }, { 260, 360, 360, 420, 260 }, 5);
            add(TestAnimationId::Smile, "SMILE", { neutral, happy, shifted(happy, 0, -2), happy, neutral }, { 260, 420, 400, 500, 280 }, 5);
            add(TestAnimationId::TiredYawn, "YAWN", { tired, almostClosed, shifted(yawn, 0, -1), sleepyHappy, tired }, { 400, 440, 850, 550, 420 }, 5);
            add(TestAnimationId::SlowBlink, "SLOW BLINK", { tired, eyesClosed, tired }, { 820, 720, 650 });
            add(TestAnimationId::HalfClosedEyes, "HALF CLOSED EYES", { tired, almostClosed, tired }, { 500, 700, 460 });
            add(TestAnimationId::SlowEyeMovement, "SLOW EYE MOVEMENT", {
                Pose{ 0, 0, 32, 12, -7, 0, 0, 0, Mouth::None, -1 },
                Pose{ 0, 0, 32, 12, 7, 0, 0, 0, Mouth::None, -1 },
                tired
            }, { 900, 900, 700 });
            add(TestAnimationId::TiredAlmostAsleep, "ALMOST ASLEEP", { tired, eyesVeryClosed, tired }, { 900, 850, 380 });
            add(TestAnimationId::SleepEyeOpen, "SLEEP EYE OPEN", { eyesClosed, eyesVeryClosed, eyesClosed }, { 420, 740, 520 });
            add(TestAnimationId::SleepZ, "ZZZ", { zSmall, zHigh, zSmall }, { 720, 850, 720 });
            add(TestAnimationId::WakeUp, "WAKE UP", { eyesClosed, halfLid, eyesClosed, halfLid, neutral }, { 500, 380, 260, 600, 400 }, 5);

        }

        const std::array<AnimationTestDefinition, 27> animationTests = {{
            { TestAnimationId::Blink, RobotState::Idle, "BLINK" },
            { TestAnimationId::LookLeft, RobotState::Idle, "LOOK LEFT" },
            { TestAnimationId::LookRight, RobotState::Idle, "LOOK RIGHT" },
            { TestAnimationId::LookUp, RobotState::Idle, "LOOK UP" },
            { TestAnimationId::LookAround, RobotState::Idle, "LOOK AROUND" },
            { TestAnimationId::Thinking, RobotState::Idle, "THINKING" },
            { TestAnimationId::IdleYawn, RobotState::Idle, "YAWN" },
            { TestAnimationId::Surprised, RobotState::Idle, "SURPRISED" },
            { TestAnimationId::Angry, RobotState::Idle, "ANGRY" },
            { TestAnimationId::Content, RobotState::Idle, "CONTENTED" },
            { TestAnimationId::Bored, RobotState::Idle, "BORED" },
            { TestAnimationId::SelfSatisfied, RobotState::Idle, "SELF SATISFIED" },
            { TestAnimationId::IdleAlmostAsleep, RobotState::Idle, "ALMOST ASLEEP" },
            { TestAnimationId::Listening, RobotState::Idle, "LISTENING" },
            { TestAnimationId::RandomCuriosity, RobotState::Idle, "RANDOM CURIOSITY" },
            { TestAnimationId::CheckSurroundings, RobotState::Idle, "CHECK SURROUNDINGS" },
            { TestAnimationId::BriefFreeze, RobotState::Idle, "BRIEF FREEZE" },
            { TestAnimationId::Hum, RobotState::Idle, "HUM" },
            { TestAnimationId::Smile, RobotState::Idle, "SMILE" },
            { TestAnimationId::TiredYawn, RobotState::Tired, "YAWN" },
            { TestAnimationId::SlowBlink, RobotState::Tired, "SLOW BLINK" },
            { TestAnimationId::HalfClosedEyes, RobotState::Tired, "HALF CLOSED EYES" },
            { TestAnimationId::SlowEyeMovement, RobotState::Tired, "SLOW EYE MOVEMENT" },
            { TestAnimationId::TiredAlmostAsleep, RobotState::Tired, "ALMOST ASLEEP" },
            { TestAnimationId::SleepEyeOpen, RobotState::Sleep, "SLEEP EYE OPEN" },
            { TestAnimationId::SleepZ, RobotState::Sleep, "ZZZ" },
            { TestAnimationId::WakeUp, RobotState::Sleep, "WAKE UP" }
        }};

        AnimationStorage createStateAnimation(const char* name, const Pose& pose)
        {
            AnimationStorage result = createAnimation(name, { pose, shifted(pose, 0, -1), pose }, { 1200, 1200, 500 }, 3, true);
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
