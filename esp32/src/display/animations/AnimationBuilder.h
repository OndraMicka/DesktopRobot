#pragma once
#include "TestAnimations.h"
#include <algorithm>
#include <cmath>
namespace desktoprobot { namespace animationdata {
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

        inline int interpolate(int start, int end, int step, int steps)
        {
            const int numerator = start * (steps - step) + end * step;
            return numerator >= 0
                ? (numerator + steps / 2) / steps
                : (numerator - steps / 2) / steps;
        }

        inline Pose interpolatePose(const Pose& start, const Pose& end, int step, int steps)
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

        inline void append(std::array<DrawCommand, MAX_COMMANDS>& commands, std::size_t& count, DrawCommand command)
        {
            if (count < commands.size())
            {
                commands[count++] = command;
            }
        }

        inline void drawSoftArc(
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

        inline void drawEye(
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

        inline void drawMouth(
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

        inline void drawPose(std::array<DrawCommand, MAX_COMMANDS>& commands, std::size_t& count, const Pose& pose)
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

        inline void createAnimation(
            AnimationStorage& result,
            const char* name,
            const std::array<Pose, 7>& poses,
            const std::array<uint32_t, 7>& durations,
            std::size_t poseCount = 3,
            bool loop = false
        )
        {
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
                            std::max<uint32_t>(1, step == 0 ? holdMs : (durations[segment] - holdMs) / (MORPH_STEPS - 1)),
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
        }

        inline void rebindAnimationStorage(AnimationStorage& storage)
        {
            for (std::size_t index = 0; index < storage.animation.frameCount; ++index)
            {
                storage.frames[index].commands = storage.commands[index].data();
            }
            storage.animation.frames = storage.frames.data();
        }

}}
