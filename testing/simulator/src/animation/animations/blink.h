#pragma once

#include "../Animation.h"

namespace desktoprobot
{
    constexpr DrawCommand blinkFrameOpen[] = {
        rectangle(25, 18, 20, 12), rectangle(83, 18, 20, 12),
        rectangle(45, 46, 38, 3)
    };
    constexpr DrawCommand blinkFrameHalf[] = {
        rectangle(25, 22, 20, 5), rectangle(83, 22, 20, 5),
        rectangle(45, 46, 38, 3)
    };
    constexpr DrawCommand blinkFrameClosed[] = {
        rectangle(25, 24, 20, 2), rectangle(83, 24, 20, 2),
        rectangle(45, 46, 38, 3)
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
