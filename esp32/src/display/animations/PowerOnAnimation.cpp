#include "PowerOnAnimation.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace desktoprobot
{
    namespace
    {
        struct Storage
        {
            std::array<std::array<DrawCommand, 3>, 77> commands{};
            std::array<Frame, 77> frames{};
            Animation animation{};

            Storage()
            {
                std::size_t index = 0;
                const auto add = [&](uint32_t duration, int eyeWidth, int eyeHeight,
                                     int mouthWidth, int mouthHeight) {
                    auto& drawing = commands[index];
                    std::size_t count = 0;
                    if (eyeWidth > 0 && eyeHeight > 0) {
                        for (const int centerX : {34, 94}) {
                            drawing[count++] = roundedRectangle(centerX - eyeWidth / 2,
                                25 - eyeHeight / 2, eyeWidth, eyeHeight,
                                std::min(eyeWidth, eyeHeight) / 2);
                        }
                    }
                    if (mouthWidth > 0 && mouthHeight > 0) {
                        drawing[count++] = roundedRectangle(64 - mouthWidth / 2,
                            51 - mouthHeight / 2, mouthWidth, mouthHeight,
                            std::min(mouthWidth, mouthHeight) / 2);
                    }
                    frames[index] = {duration, drawing.data(), count};
                    ++index;
                };
                const auto ease = [](int step, int steps) {
                    const double t = static_cast<double>(step) / steps;
                    return t * t * (3.0 - 2.0 * t);
                };

                // Give the arrival a quiet beat before anything is visible.
                add(900, 0, 0, 0, 0);
                // Tiny seeds slowly unfold sideways into soft, closed eyelids.
                for (int step = 1; step <= 25; ++step) {
                    const double t = ease(step, 25);
                    const int width = static_cast<int>(std::lround(32 * t));
                    const int height = std::max(1, static_cast<int>(std::lround(3 * t)));
                    add(40, width, height, 0, 0);
                }
                // Open the completed lines symmetrically up and down in one
                // uninterrupted motion. No partial blink or overshoot.
                for (int step = 1; step <= 50; ++step) {
                    const int eyeHeight = static_cast<int>(std::lround(3 + 25 * ease(step, 50)));
                    // The mouth starts with the vertical opening and unfolds
                    // in 400 ms while the eyes take their full two seconds.
                    const double mouth = ease(std::min(step, 10), 10);
                    const int mouthWidth = static_cast<int>(std::lround(20 * mouth));
                    const int mouthHeight = std::max(1, static_cast<int>(std::lround(5 * mouth)));
                    add(40, 32, eyeHeight, mouthWidth, mouthHeight);
                }
                add(450, 32, 28, 20, 5);
                animation = {"POWER ON", frames.data(), index, false};
            }
        };
    }

    const Animation& getPowerOnAnimation()
    {
        static const Storage storage;
        return storage.animation;
    }
}
