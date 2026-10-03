#pragma once

#include "../Animation.h"

namespace desktoprobot
{
    constexpr DrawCommand smileFrame1[] = {
        rectangle(25, 18, 20, 12), rectangle(83, 18, 20, 12),
        line(45, 45, 53, 50), line(53, 50, 64, 52),
        line(64, 52, 75, 50), line(75, 50, 83, 45)
    };
    constexpr DrawCommand smileFrame2[] = {
        rectangle(25, 17, 20, 13), rectangle(83, 17, 20, 13),
        line(45, 44, 53, 51), line(53, 51, 64, 53),
        line(64, 53, 75, 51), line(75, 51, 83, 44)
    };
    constexpr Frame smileFrames[] = {
        { 180, smileFrame1, sizeof(smileFrame1) / sizeof(smileFrame1[0]) },
        { 180, smileFrame2, sizeof(smileFrame2) / sizeof(smileFrame2[0]) }
    };
    constexpr Animation smileAnimation = {
        "smile", smileFrames, sizeof(smileFrames) / sizeof(smileFrames[0]), true
    };
}
