#pragma once

#include "smile.h"

namespace desktoprobot
{
    constexpr Animation happyAnimation = {
        "happy", smileFrames, sizeof(smileFrames) / sizeof(smileFrames[0]), true
    };
}
