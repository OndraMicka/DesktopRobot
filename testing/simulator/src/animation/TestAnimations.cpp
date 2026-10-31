// CODEX TEST
 #include "TestAnimations.h"
#include "../../../../esp32/src/display/animations/TouchAnimationBuilder.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>

namespace desktoprobot
{
    namespace
    {
        constexpr std::size_t MAX_FRAMES = 49;
        constexpr std::size_t MAX_COMMANDS = 80;
        constexpr int NO_Z = -100;

        enum class Mouth : uint8_t
        {
            None,
            Smile,
            Open,
            Frown
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
            int mouthWidth = 0;
            int mouthHeight = 0;
            int eyeRoll = 0;
        };

        struct AnimationStorage
        {
            std::array<std::array<DrawCommand, MAX_COMMANDS>, MAX_FRAMES> commands{};
            std::array<Frame, MAX_FRAMES> frames{};
            std::array<Pose, MAX_FRAMES> poses{};
            Animation animation{};
        };

        // One OLED character: large solid eyes, fixed spacing and a bold, simple mouth.
        // Gaze moves the whole eye; closed eyes are always straight, rounded bars.
        constexpr Pose neutral{ 0, 0, 32, 28, 0, 0, 0, 0, Mouth::None, NO_Z };
        constexpr Pose glanceLeft{ 0, 0, 32, 28, -7, 0, 0, 0, Mouth::None, NO_Z };
        constexpr Pose glanceRight{ 0, 0, 32, 28, 7, 0, 0, 0, Mouth::None, NO_Z };
        constexpr Pose glanceUp{ 0, 0, 32, 28, 0, -6, 0, 0, Mouth::None, NO_Z };
        constexpr Pose thinking{ 0, 0, 32, 16, 5, -4, 0, 0, Mouth::None, NO_Z };
        constexpr Pose halfLid{ 0, 0, 32, 14, 0, 0, 0, 0, Mouth::None, NO_Z };
        constexpr Pose tired{ 0, 0, 32, 12, 0, 0, 0, 0, Mouth::None, NO_Z };
        constexpr Pose almostClosed{ 0, 0, 32, 5, 0, 0, 0, 0, Mouth::None, NO_Z };
        constexpr Pose eyesClosed{ 0, 0, 32, 3, 0, 0, 0, 0, Mouth::None, NO_Z };
        constexpr Pose eyesVeryClosed{ 0, 0, 32, 2, 0, 0, 0, 0, Mouth::None, NO_Z };
        constexpr Pose happy{ 0, 0, 32, 22, 0, 0, 0, 0, Mouth::Smile, NO_Z };
        constexpr Pose yawn{ 0, 0, 32, 9, 0, 0, 0, 0, Mouth::Open, NO_Z };
        constexpr Pose angry{ 0, 0, 32, 18, 0, 0, 5, 0, Mouth::None, NO_Z };
        constexpr Pose bored{ 0, 0, 32, 12, 3, 1, 0, 0, Mouth::None, NO_Z };
        constexpr Pose zSmall{ 0, 0, 32, 3, 0, 0, 0, 0, Mouth::None, 10 };
        constexpr Pose zHigh{ 0, 0, 32, 3, 0, 0, 0, 0, Mouth::None, -14 };

        constexpr Pose shifted(Pose pose, int x, int y)
        {
            pose.eyeOffsetX += x;
            pose.eyeOffsetY += y;
            return pose;
        }

        constexpr Pose mouthSized(Pose pose, int width, int height)
        {
            pose.mouthWidth = width;
            pose.mouthHeight = height;
            return pose;
        }

        constexpr Pose zAt(int y)
        {
            Pose pose = zSmall;
            pose.zOffset = y;
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
            const int zOffset = start.zOffset == NO_Z || end.zOffset == NO_Z
                ? NO_Z : interpolate(start.zOffset, end.zOffset, step, steps);
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
                zOffset,
                interpolate(start.eyeSpacing, end.eyeSpacing, step, steps),
                interpolate(start.mouthWidth, end.mouthWidth, step, steps),
                interpolate(start.mouthHeight, end.mouthHeight, step, steps),
                interpolate(start.eyeRoll, end.eyeRoll, step, steps)
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
            int tilt
        )
        {
            const int x = centerX - width / 2;
            const int y = centerY - height / 2;
            if (height <= 5)
            {
                append(commands, count, roundedRectangle(x, centerY - 2, width, 5, 2));
                return;
            }

            const int cornerRadius = std::min(width, height) / 2;
            append(commands, count, roundedRectangle(x, y, width, height, cornerRadius));

            if (tilt != 0)
            {
                // Trim the eye itself: the upper lid slopes down toward the nose.
                const int slope = clamp(tilt, -6, 6);
                const int depth = 2 * (slope < 0 ? -slope : slope);
                for (int column = 0; column < width; ++column)
                {
                    const int cut = depth * (slope > 0 ? column : width - 1 - column) / (width - 1);
                    append(commands, count, line(x + column, y, x + column, y + cut, false));
                }
            }
        }

        void drawMouth(
            std::array<DrawCommand, MAX_COMMANDS>& commands,
            std::size_t& count,
            Mouth mouth,
            int offsetX,
            int offsetY,
            int width,
            int height
        )
        {
            const std::size_t firstCommand = count;
            switch (mouth)
            {
            case Mouth::None:
                append(commands, count, roundedRectangle(54, 49, 20, 5, 2));
                break;
            case Mouth::Smile:
                drawSoftArc(commands, count, 51, 48, 27, 3, 2);
                break;
            case Mouth::Open:
                width = width > 0 ? width : 14;
                height = height > 0 ? height : 17;
                append(commands, count, roundedRectangle(
                    64 - width / 2, 51 - height / 2, width, height, std::min(width, height) / 2
                ));
                break;
            case Mouth::Frown:
                drawSoftArc(commands, count, 53, 52, 23, -3, 2);
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
                commands, count, leftCenterX, eyeCenterY - pose.eyeRoll,
                pose.eyeWidth, pose.eyeHeight, pose.eyeTilt
            );
            drawEye(
                commands, count, rightCenterX, eyeCenterY + pose.eyeRoll,
                pose.eyeWidth,
                clamp(pose.eyeHeight + pose.rightEyeHeightOffset, 2, 36),
                -pose.eyeTilt
            );
            drawMouth(commands, count, pose.mouth, pose.eyeOffsetX, pose.eyeOffsetY,
                pose.mouthWidth, pose.mouthHeight);

            // Negative Y intentionally lets the one Z be clipped as it leaves the screen.
            if (pose.zOffset != NO_Z && pose.zOffset > -13)
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
            const std::array<Pose, 7>& poses,
            const std::array<uint32_t, 7>& durations,
            std::size_t poseCount = 3,
            bool loop = false
        )
        {
            AnimationStorage result{};
            std::size_t frameCount = 0;

            if (poseCount == 1)
            {
                result.poses[0] = poses[0];
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
                        result.poses[frameCount] = pose;
                        drawPose(result.commands[frameCount], commandCount, pose);
                        // Pause on the expression, then move through the next gesture.
                        const bool floatingZ = poses[segment].zOffset != NO_Z
                            && poses[segment + 1].zOffset != NO_Z;
                        const uint32_t holdMs = durations[segment] / (floatingZ ? MORPH_STEPS : 3);
                        result.frames[frameCount] = {
                            std::max(1U, step == 0 ? holdMs : (durations[segment] - holdMs) / (MORPH_STEPS - 1)),
                            result.commands[frameCount].data(),
                            commandCount
                        };
                        ++frameCount;
                    }
                }
                std::size_t commandCount = 0;
                result.poses[frameCount] = poses[poseCount - 1];
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

        AnimationStorage createDizzyAnimation(const Pose& start)
        {
            AnimationStorage result{};
            constexpr double PI = 3.141592653589793;
            constexpr std::size_t SHAKE_OFF_START = 37;
            constexpr std::array<int, 12> shakeOffsets{ 0, -3, 5, -5, 4, -3, 2, -1, 0, 0, 0, 0 };
            constexpr std::array<int, 12> shakeEyeHeights{ 18, 14, 7, 5, 5, 7, 10, 16, 23, 28, 28, 28 };
            for (std::size_t index = 0; index < MAX_FRAMES; ++index)
            {
                const double time = index < SHAKE_OFF_START ? index * 0.08
                    : SHAKE_OFF_START * 0.08 + (index - SHAKE_OFF_START) * 0.09;
                const double phase = 2.0 * PI * time / 1.35;
                // Keep circling until the shake starts; shaking clears the dizziness.
                const double fade = std::clamp((44.0 - index) / 7.0, 0.0, 1.0);
                const double amplitude = fade * fade * (3.0 - 2.0 * fade);
                const auto wave = [amplitude](double value) {
                    return static_cast<int>(std::lround(amplitude * value));
                };
                Pose orbit = neutral;
                // The face circles while the eyes lag behind and the head rolls.
                orbit.eyeOffsetX = wave(3.0 * std::sin(phase));
                orbit.eyeOffsetY = wave(2.0 * std::cos(phase));
                orbit.gazeX = wave(4.0 * std::sin(phase - 0.7));
                orbit.gazeY = wave(2.0 * std::cos(phase - 0.7));
                orbit.eyeRoll = wave(3.0 * std::sin(phase));
                orbit.eyeHeight = 28 - wave(10.0);
                // Blend from the exact shake pose, regardless of release timing.
                const int entryStep = static_cast<int>(std::min(index, std::size_t{6}));
                Pose pose = interpolatePose(start, orbit, entryStep, 6);
                uint32_t durationMs = 80;
                if (index >= SHAKE_OFF_START)
                {
                    // Layer the shake over the ongoing dizzy motion, then
                    // open the eyes as both movements settle together.
                    const std::size_t step = index - SHAKE_OFF_START;
                    pose = shifted(orbit, shakeOffsets[step], 0);
                    pose.eyeHeight = shakeEyeHeights[step];
                    durationMs = index + 1 == MAX_FRAMES ? 240 : 90;
                }
                result.poses[index] = pose;
                std::size_t count = 0;
                drawPose(result.commands[index], count, pose);
                result.frames[index] = { durationMs, result.commands[index].data(), count };
            }
            result.animation = { "DIZZY", result.frames.data(), MAX_FRAMES, false };
            return result;
        }

        void initializeAnimations(
            std::array<AnimationStorage, static_cast<std::size_t>(TestAnimationId::Count)>& result
        )
        {
            const auto add = [&result](
                TestAnimationId id,
                const char* name,
                const std::array<Pose, 7>& poses,
                const std::array<uint32_t, 7>& durations,
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

            const Pose yawnStart = mouthSized(shifted(yawn, -2, 2), 10, 10);
            Pose yawnStretch = mouthSized(shifted(yawn, -3, -4), 18, 22);
            yawnStretch.eyeHeight = 3;
            yawnStretch.eyeWidth = 34;
            const Pose yawnRelease = mouthSized(shifted(yawn, 2, -1), 12, 14);
            Pose boredLeft = bored;
            boredLeft.gazeX = -3;
            Pose boredBlink = boredLeft;
            boredBlink.eyeHeight = 7;
            boredBlink.rightEyeHeightOffset = 0;

            add(TestAnimationId::Blink, "BLINK", { neutral, eyesClosed, neutral, eyesClosed, neutral }, { 140, 90, 220, 90, 180 }, 5);
            add(TestAnimationId::LookLeft, "LOOK LEFT", { neutral, glanceLeft, shifted(glanceLeft, -2, 1), glanceLeft, neutral }, { 240, 420, 300, 220, 240 }, 5);
            add(TestAnimationId::LookRight, "LOOK RIGHT", { neutral, glanceRight, shifted(glanceRight, 2, 1), glanceRight, neutral }, { 240, 420, 300, 220, 240 }, 5);
            add(TestAnimationId::LookUp, "LOOK UP", { neutral, glanceUp, shifted(glanceUp, 0, 2), glanceUp, neutral }, { 240, 440, 260, 220, 260 }, 5);
            add(TestAnimationId::LookAround, "LOOK AROUND", { neutral, glanceLeft, glanceUp, glanceRight, neutral }, { 220, 420, 320, 420, 240 }, 5);
            add(TestAnimationId::Thinking, "THINKING", { neutral, thinking, shifted(thinking, -2, 2), thinking, neutral }, { 280, 600, 420, 500, 260 }, 5);
            add(TestAnimationId::IdleYawn, "YAWN", {
                neutral, shifted(halfLid, 0, 2), yawnStart, yawnStretch,
                yawnRelease, shifted(tired, 1, 2), neutral
            }, { 260, 420, 420, 740, 420, 360, 220 }, 7);
            Pose braced = angry;
            braced.eyeHeight = 11;
            braced.eyeTilt = 3;
            add(TestAnimationId::Shaking, "SHAKING", {
                shifted(braced, -3, 1), shifted(braced, 3, -1),
                shifted(braced, -2, -1), shifted(braced, 2, 1), shifted(braced, -3, 1)
            }, { 80, 80, 80, 80, 1 }, 5);
            result[static_cast<std::size_t>(TestAnimationId::Shaking)].animation.loop = true;
            auto& dizzy = result[static_cast<std::size_t>(TestAnimationId::Dizzy)];
            dizzy = createDizzyAnimation(neutral);
            rebindAnimationStorage(dizzy);
            add(TestAnimationId::Angry, "ANGRY", {
                neutral, angry, angry, neutral
            }, { 220, 900, 700, 300 }, 4);
            add(TestAnimationId::Smile, "SMILE", {
                neutral, happy, shifted(happy, 0, -1), happy, neutral
            }, { 320, 750, 650, 900, 320 }, 5);
            add(TestAnimationId::Bored, "BORED", {
                neutral, bored, shifted(bored, 0, 1), boredLeft,
                shifted(boredBlink, 0, 1), boredLeft, neutral
            }, { 400, 800, 700, 800, 600, 800, 320 }, 7);
            add(TestAnimationId::Listening, "LISTENING", { neutral, glanceRight, shifted(glanceRight, 0, -2), glanceRight, neutral }, { 260, 500, 400, 500, 260 }, 5);
            add(TestAnimationId::RandomCuriosity, "RANDOM CURIOSITY", { neutral, glanceLeft, glanceUp, glanceRight, neutral }, { 200, 260, 300, 280, 220 }, 5);
            add(TestAnimationId::TiredYawn, "YAWN", {
                tired, shifted(almostClosed, 0, 2), yawnStart, yawnStretch,
                yawnRelease, shifted(eyesClosed, 1, 3), tired
            }, { 400, 520, 480, 900, 500, 520, 400 }, 7);
            add(TestAnimationId::SlowBlink, "SLOW BLINK", { tired, eyesClosed, tired }, { 820, 720, 650 });
            add(TestAnimationId::SlowEyeMovement, "SLOW EYE MOVEMENT", {
                tired,
                Pose{ 0, 0, 32, 12, -7, 0, 0, 0, Mouth::None, NO_Z },
                tired,
                Pose{ 0, 0, 32, 12, 7, 0, 0, 0, Mouth::None, NO_Z },
                tired
            }, { 360, 700, 500, 700, 360 }, 5);
            // Open one eye far enough to be visibly different from the closed bar.
            Pose sleepPeek = eyesClosed;
            sleepPeek.eyeHeight = 14;
            sleepPeek.rightEyeHeightOffset = -11;
            sleepPeek.gazeX = -3;
            add(TestAnimationId::SleepEyeOpen, "SLEEP EYE OPEN", {
                eyesClosed, sleepPeek, shifted(sleepPeek, 0, -1), sleepPeek, eyesClosed
            }, { 360, 580, 480, 400, 360 }, 5);
            add(TestAnimationId::SleepZ, "ZZZ", {
                zSmall, zAt(6), zAt(2), zAt(-2), zAt(-6), zAt(-10), zHigh
            }, { 600, 600, 600, 600, 600, 600, 250 }, 7);
            add(TestAnimationId::WakeUp, "WAKE UP", { eyesClosed, halfLid, eyesClosed, halfLid, neutral }, { 500, 380, 260, 600, 400 }, 5);
            add(TestAnimationId::TiredToSleep, "TIRED TO SLEEP", {
                tired, shifted(almostClosed, 0, 1), tired,
                shifted(almostClosed, 0, 1), eyesClosed
            }, { 420, 560, 340, 680, 600 }, 5);
            add(TestAnimationId::IdleToTired, "IDLE TO TIRED", {
                neutral, halfLid, yawnStart, yawnStretch, yawnRelease, halfLid, tired
            }, { 350, 420, 350, 650, 400, 500, 450 }, 7);
            add(TestAnimationId::IdleToSleep, "IDLE TO SLEEP", {
                neutral, halfLid, yawnStretch, tired, almostClosed, eyesClosed, eyesClosed
            }, { 350, 500, 700, 600, 750, 500, 600 }, 7);
            add(TestAnimationId::TiredToIdle, "TIRED TO IDLE", {
                tired, eyesClosed, halfLid, neutral, glanceLeft, glanceRight, neutral
            }, { 320, 180, 450, 350, 320, 320, 300 }, 7);
            add(TestAnimationId::SleepToTired, "SLEEP TO TIRED", {
                eyesClosed, almostClosed, eyesClosed, halfLid, tired
            }, { 500, 500, 280, 650, 500 }, 5);


        }

        const std::array<AnimationTestDefinition, static_cast<std::size_t>(TestAnimationId::Count)> animationTests = {{
            { TestAnimationId::Blink, RobotState::Idle, "BLINK" },
            { TestAnimationId::LookLeft, RobotState::Idle, "LOOK LEFT" },
            { TestAnimationId::LookRight, RobotState::Idle, "LOOK RIGHT" },
            { TestAnimationId::LookUp, RobotState::Idle, "LOOK UP" },
            { TestAnimationId::LookAround, RobotState::Idle, "LOOK AROUND" },
            { TestAnimationId::Thinking, RobotState::Idle, "THINKING" },
            { TestAnimationId::IdleYawn, RobotState::Idle, "YAWN" },
            { TestAnimationId::Shaking, RobotState::Idle, "SHAKE - HOLD" },
            { TestAnimationId::Dizzy, RobotState::Idle, "DIZZY" },
            { TestAnimationId::Angry, RobotState::Idle, "ANGRY" },
            { TestAnimationId::Smile, RobotState::Idle, "SMILE" },
            { TestAnimationId::Bored, RobotState::Idle, "BORED" },
            { TestAnimationId::Listening, RobotState::Idle, "LISTENING" },
            { TestAnimationId::RandomCuriosity, RobotState::Idle, "RANDOM CURIOSITY" },
            { TestAnimationId::TiredYawn, RobotState::Tired, "YAWN" },
            { TestAnimationId::SlowBlink, RobotState::Tired, "SLOW BLINK" },
            { TestAnimationId::SlowEyeMovement, RobotState::Tired, "SLOW EYE MOVEMENT" },
            { TestAnimationId::SleepEyeOpen, RobotState::Sleep, "SLEEP EYE OPEN" },
            { TestAnimationId::SleepZ, RobotState::Sleep, "ZZZ" },
            { TestAnimationId::WakeUp, RobotState::Sleep, "WAKE UP - TO IDLE", true, RobotState::Idle },
            { TestAnimationId::TiredToSleep, RobotState::Tired, "TIRED TO SLEEP", true, RobotState::Sleep },
            { TestAnimationId::IdleToTired, RobotState::Idle, "IDLE TO TIRED", true, RobotState::Tired },
            { TestAnimationId::IdleToSleep, RobotState::Idle, "IDLE TO SLEEP", true, RobotState::Sleep },
            { TestAnimationId::TiredToIdle, RobotState::Tired, "TIRED TO IDLE", true, RobotState::Idle },
            { TestAnimationId::SleepToTired, RobotState::Sleep, "SLEEP TO TIRED", true, RobotState::Tired }
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

    const std::array<AnimationTestDefinition, static_cast<std::size_t>(TestAnimationId::Count)>& getAnimationTestDefinitions()
    {
        return animationTests;
    }

    const Animation& getTestAnimation(TestAnimationId id)
    {
        return getAnimations()[static_cast<std::size_t>(id)].animation;
    }

    const Animation& getShakeRecoveryAnimation(const Frame& releaseFrame)
    {
        const auto& shake = getAnimations()[static_cast<std::size_t>(TestAnimationId::Shaking)];
        for (std::size_t index = 0; index < shake.animation.frameCount; ++index)
        {
            if (&releaseFrame == &shake.frames[index])
            {
                static AnimationStorage recovery{};
                recovery = createDizzyAnimation(shake.poses[index]);
                rebindAnimationStorage(recovery);
                return recovery.animation;
            }
        }
        return getTestAnimation(TestAnimationId::Dizzy);
    }

    const Animation& getTouchReturnAnimation(const Frame& releaseFrame)
    {
        if (const auto* custom = tryTouchReturn(releaseFrame)) return *custom;
        Pose start = neutral;
        for (const auto& source : getAnimations())
            for (std::size_t i = 0; i < source.animation.frameCount; ++i)
                if (&releaseFrame == &source.frames[i]) start = source.poses[i];
        static AnimationStorage recovery;
        recovery = createAnimation("TOUCH RETURN TIRED", {start, tired}, {360, 1}, 2);
        rebindAnimationStorage(recovery);
        return recovery.animation;
    }

    const AnimationTestDefinition* getStateTransition(RobotState from, RobotState to)
    {
        for (const auto& definition : animationTests)
        {
            if (definition.transition && definition.state == from && definition.targetState == to)
                return &definition;
        }
        return nullptr;
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
