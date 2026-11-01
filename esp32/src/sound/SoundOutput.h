#pragma once
#include "SoundData.h"

namespace desktoprobot {
class SoundOutput {
public:
    virtual ~SoundOutput() = default;
    virtual void play(SoundEffect effect, bool loop = false) = 0;
    virtual void stop() = 0;
    virtual bool isPlaying() const = 0;
};
}
