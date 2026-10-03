#pragma once

#include "../Animation.h"

namespace desktoprobot
{
    constexpr DrawCommand surprisedFrame1[] = {
        rectangle(25, 16, 20, 16), rectangle(83, 16, 20, 16),
        circle(64, 49, 5)
    };
    constexpr DrawCommand surprisedFrame2[] = {
        rectangle(24, 15, 22, 18), rectangle(82, 15, 22, 18),
        circle(64, 49, 6)
    };
    constexpr Frame surprisedFrames[] = {
        { 120, surprisedFrame1, sizeof(surprisedFrame1) / sizeof(surprisedFrame1[0]) },
        { 160, surprisedFrame2, sizeof(surprisedFrame2) / sizeof(surprisedFrame2[0]) },
        { 120, surprisedFrame1, sizeof(surprisedFrame1) / sizeof(surprisedFrame1[0]) }
    };
    constexpr Animation surprisedAnimation = {
        "surprised", surprisedFrames, sizeof(surprisedFrames) / sizeof(surprisedFrames[0]), false
    };
}
