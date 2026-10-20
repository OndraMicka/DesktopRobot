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
        bool play(const SoundDefinition& sound, std::string& error, bool loop = false) override;
        void stop() override;

    private:
        static void audioCallback(void* userdata, Uint8* stream, int length);
        bool generateTone(
            std::vector<int16_t>& samples,
            float startFrequencyHz,
            float endFrequencyHz,
            uint32_t durationMs
        );

        SDL_AudioDeviceID device_ = 0;
        double phase_ = 0.0;
        std::vector<int16_t> samples_;
        std::size_t sampleIndex_ = 0;
        bool loop_ = false;
    };
}
