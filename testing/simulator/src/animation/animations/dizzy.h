#pragma once

#include "../Animation.h"

namespace desktoprobot
{
    constexpr DrawCommand dizzyFrame1[] = {
        line(25, 18, 45, 30), line(45, 18, 25, 30),
        line(83, 18, 103, 30), line(103, 18, 83, 30),
        line(48, 48, 57, 52), line(57, 52, 70, 48), line(70, 48, 80, 52)
    };
    constexpr DrawCommand dizzyFrame2[] = {
        line(25, 18, 45, 30), line(45, 18, 25, 30),
        line(83, 18, 103, 30), line(103, 18, 83, 30),
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
