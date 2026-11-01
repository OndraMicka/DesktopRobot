#pragma once
#include "../../../../esp32/src/sound/SoundOutput.h"

namespace desktoprobot
{
    class AudioBackend;

    class SoundPlayer final : public SoundOutput
    {
    public:
        explicit SoundPlayer(AudioBackend& backend);

        void play(SoundEffect effect, bool loop = false) override;
        void stop() override;
        bool isPlaying() const override;

    private:
        AudioBackend& backend_;
    };
}
