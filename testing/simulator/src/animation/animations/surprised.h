#pragma once

#include "../Animation.h"

namespace desktoprobot
{
    constexpr DrawCommand surprisedFrame1[] = {
        roundedRectangle(16, 10, 38, 34, 13), roundedRectangle(74, 10, 38, 34, 13),
        roundedRectangle(31, 21, 8, 16, 4, false), roundedRectangle(89, 21, 8, 16, 4, false),
        circle(64, 50, 5, true)
    };
    constexpr DrawCommand surprisedFrame2[] = {
        roundedRectangle(15, 8, 40, 37, 15), roundedRectangle(73, 8, 40, 37, 15),
        roundedRectangle(31, 20, 8, 17, 4, false), roundedRectangle(89, 20, 8, 17, 4, false),
        circle(64, 50, 6, true)
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
