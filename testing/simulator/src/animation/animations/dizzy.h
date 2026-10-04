#pragma once

#include "../Animation.h"

namespace desktoprobot
{
    constexpr DrawCommand dizzyFrame1[] = {
        roundedRectangle(17, 14, 36, 27, 10), roundedRectangle(75, 14, 36, 27, 10),
        line(27, 22, 43, 34, false), line(43, 22, 27, 34, false),
        line(85, 22, 101, 34, false), line(101, 22, 85, 34, false),
        line(48, 48, 57, 52), line(57, 52, 70, 48), line(70, 48, 80, 52)
    };
    constexpr DrawCommand dizzyFrame2[] = {
        roundedRectangle(17, 14, 36, 27, 10), roundedRectangle(75, 14, 36, 27, 10),
        line(27, 22, 43, 34, false), line(43, 22, 27, 34, false),
        line(85, 22, 101, 34, false), line(101, 22, 85, 34, false),
        line(48, 52, 58, 48), line(58, 48, 70, 52), line(70, 52, 80, 48)
    };
    constexpr Frame dizzyFrames[] = {
        { 120, dizzyFrame1, sizeof(dizzyFrame1) / sizeof(dizzyFrame1[0]) },
        { 120, dizzyFrame2, sizeof(dizzyFrame2) / sizeof(dizzyFrame2[0]) },
        { 120, dizzyFrame1, sizeof(dizzyFrame1) / sizeof(dizzyFrame1[0]) },
        { 120, dizzyFrame2, sizeof(dizzyFrame2) / sizeof(dizzyFrame2[0]) }
    };
    constexpr Animation dizzyAnimation = {
        "dizzy", dizzyFrames, sizeof(dizzyFrames) / sizeof(dizzyFrames[0]), false
    };
}
