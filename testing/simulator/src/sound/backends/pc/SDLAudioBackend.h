#pragma once

#include "sound/AudioBackend.h"

#include <SDL2/SDL.h>

#include <cstdint>
#include <string>
#include <vector>

namespace desktoprobot
{
    class SDLAudioBackend final : public AudioBackend
    {
    public:
        ~SDLAudioBackend() override;

        bool initialize();
        void shutdown();
        bool play(const SoundDefinition& sound, std::string& error) override;

    private:
        bool generateTone(
            std::vector<int16_t>& samples,
            float startFrequencyHz,
            float endFrequencyHz,
            uint32_t durationMs
        );

        SDL_AudioDeviceID device_ = 0;
        double phase_ = 0.0;
    };
}
