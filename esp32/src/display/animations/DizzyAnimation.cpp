#include "DizzyAnimation.h"
namespace desktoprobot {
using namespace animationdata;
void DizzyAnimation::build(AnimationStorage& result, const Pose& start)
        {
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
        }

}
