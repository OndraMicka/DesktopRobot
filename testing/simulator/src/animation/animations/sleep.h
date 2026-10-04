#pragma once

#include "../Animation.h"

namespace desktoprobot
{
    constexpr DrawCommand sleepFrame1[] = {
        roundedRectangle(17, 25, 36, 6, 3), roundedRectangle(75, 25, 36, 6, 3),
        line(56, 50, 72, 50)
    };
    constexpr DrawCommand sleepFrame2[] = {
        roundedRectangle(17, 26, 36, 5, 2), roundedRectangle(75, 26, 36, 5, 2),
        line(56, 50, 72, 50)
    };
    constexpr Frame sleepFrames[] = {
        { 350, sleepFrame1, sizeof(sleepFrame1) / sizeof(sleepFrame1[0]) },
        { 350, sleepFrame2, sizeof(sleepFrame2) / sizeof(sleepFrame2[0]) }
    };
    constexpr Animation sleepAnimation = {
        "sleep", sleepFrames, sizeof(sleepFrames) / sizeof(sleepFrames[0]), true
    };
}
