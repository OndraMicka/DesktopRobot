#pragma once

#include "../Animation.h"

namespace desktoprobot
{
    constexpr DrawCommand sleepFrame1[] = {
        line(25, 24, 45, 24), line(83, 24, 103, 24),
        rectangle(58, 49, 12, 2)
    };
    constexpr DrawCommand sleepFrame2[] = {
        line(25, 25, 45, 25), line(83, 25, 103, 25),
        rectangle(57, 49, 14, 2)
    };
    constexpr Frame sleepFrames[] = {
        { 350, sleepFrame1, sizeof(sleepFrame1) / sizeof(sleepFrame1[0]) },
        { 350, sleepFrame2, sizeof(sleepFrame2) / sizeof(sleepFrame2[0]) }
    };
    constexpr Animation sleepAnimation = {
        "sleep", sleepFrames, sizeof(sleepFrames) / sizeof(sleepFrames[0]), true
    };
}
