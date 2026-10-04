#pragma once

#include "../Animation.h"

namespace desktoprobot
{
    constexpr DrawCommand idleFrame1[] = {
        roundedRectangle(17, 14, 36, 27, 10), roundedRectangle(75, 14, 36, 27, 10),
        roundedRectangle(31, 23, 8, 15, 4, false), roundedRectangle(89, 23, 8, 15, 4, false),
        roundedRectangle(53, 49, 22, 3, 1)
    };
    constexpr DrawCommand idleFrame2[] = {
        roundedRectangle(17, 14, 36, 27, 10), roundedRectangle(75, 14, 36, 27, 10),
        roundedRectangle(31, 23, 8, 15, 4, false), roundedRectangle(89, 23, 8, 15, 4, false),
        roundedRectangle(53, 49, 22, 3, 1)
    };
    constexpr Frame idleFrames[] = {
        { 500, idleFrame1, sizeof(idleFrame1) / sizeof(idleFrame1[0]) },
        { 500, idleFrame2, sizeof(idleFrame2) / sizeof(idleFrame2[0]) }
    };
    constexpr Animation idleAnimation = {
        "idle", idleFrames, sizeof(idleFrames) / sizeof(idleFrames[0]), true
    };
}
