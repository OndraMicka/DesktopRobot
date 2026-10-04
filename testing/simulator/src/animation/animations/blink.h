#pragma once

#include "../Animation.h"

namespace desktoprobot
{
    constexpr DrawCommand blinkFrameOpen[] = {
        roundedRectangle(17, 14, 36, 27, 10), roundedRectangle(75, 14, 36, 27, 10),
        roundedRectangle(31, 23, 8, 15, 4, false), roundedRectangle(89, 23, 8, 15, 4, false),
        roundedRectangle(53, 49, 22, 3, 1)
    };
    constexpr DrawCommand blinkFrameHalf[] = {
        roundedRectangle(17, 19, 36, 18, 8), roundedRectangle(75, 19, 36, 18, 8),
        roundedRectangle(31, 26, 8, 7, 3, false), roundedRectangle(89, 26, 8, 7, 3, false),
        roundedRectangle(53, 49, 22, 3, 1)
    };
    constexpr DrawCommand blinkFrameClosed[] = {
        roundedRectangle(17, 26, 36, 5, 2), roundedRectangle(75, 26, 36, 5, 2),
        roundedRectangle(57, 50, 14, 2, 1)
    };
    constexpr Frame blinkFrames[] = {
        { 100, blinkFrameOpen, sizeof(blinkFrameOpen) / sizeof(blinkFrameOpen[0]) },
        { 70, blinkFrameHalf, sizeof(blinkFrameHalf) / sizeof(blinkFrameHalf[0]) },
        { 80, blinkFrameClosed, sizeof(blinkFrameClosed) / sizeof(blinkFrameClosed[0]) },
        { 70, blinkFrameHalf, sizeof(blinkFrameHalf) / sizeof(blinkFrameHalf[0]) },
        { 100, blinkFrameOpen, sizeof(blinkFrameOpen) / sizeof(blinkFrameOpen[0]) }
    };
    constexpr Animation blinkAnimation = {
        "blink", blinkFrames, sizeof(blinkFrames) / sizeof(blinkFrames[0]), false
    };
}
