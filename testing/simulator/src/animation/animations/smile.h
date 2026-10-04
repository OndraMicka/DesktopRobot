#pragma once

#include "../Animation.h"

namespace desktoprobot
{
    constexpr DrawCommand smileFrame1[] = {
        roundedRectangle(17, 14, 36, 27, 10), roundedRectangle(75, 14, 36, 27, 10),
        roundedRectangle(31, 23, 8, 15, 4, false), roundedRectangle(89, 23, 8, 15, 4, false),
        line(49, 46, 56, 51), line(56, 51, 64, 53),
        line(64, 53, 72, 51), line(72, 51, 79, 46)
    };
    constexpr DrawCommand smileFrame2[] = {
        roundedRectangle(17, 15, 36, 25, 10), roundedRectangle(75, 15, 36, 25, 10),
        roundedRectangle(31, 22, 8, 14, 4, false), roundedRectangle(89, 22, 8, 14, 4, false),
        line(49, 45, 56, 51), line(56, 51, 64, 54),
        line(64, 54, 72, 51), line(72, 51, 79, 45)
    };
    constexpr Frame smileFrames[] = {
        { 180, smileFrame1, sizeof(smileFrame1) / sizeof(smileFrame1[0]) },
        { 180, smileFrame2, sizeof(smileFrame2) / sizeof(smileFrame2[0]) }
    };
    constexpr Animation smileAnimation = {
        "smile", smileFrames, sizeof(smileFrames) / sizeof(smileFrames[0]), true
    };
}
