#pragma once

#include "../Animation.h"

namespace desktoprobot
{
    constexpr DrawCommand idleFrame1[] = {
        rectangle(25, 18, 20, 12), rectangle(83, 18, 20, 12),
        rectangle(45, 46, 38, 3)
    };
    constexpr DrawCommand idleFrame2[] = {
        rectangle(25, 18, 20, 12), rectangle(83, 18, 20, 12),
        rectangle(45, 46, 38, 3)
    };
    constexpr Frame idleFrames[] = {
        { 500, idleFrame1, sizeof(idleFrame1) / sizeof(idleFrame1[0]) },
        { 500, idleFrame2, sizeof(idleFrame2) / sizeof(idleFrame2[0]) }
    };
    constexpr Animation idleAnimation = {
        "idle", idleFrames, sizeof(idleFrames) / sizeof(idleFrames[0]), true
    };
}
