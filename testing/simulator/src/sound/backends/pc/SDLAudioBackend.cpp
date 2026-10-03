#include "sound/backends/pc/SDLAudioBackend.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace desktoprobot
{
    namespace
    {
        constexpr int SAMPLE_RATE = 44100;
        constexpr double PI = 3.14159265358979323846;
        constexpr std::size_t MAX_SAMPLE_COUNT =
            std::numeric_limits<Uint32>::max() / sizeof(int16_t);

        uint64_t getSampleCount(uint32_t durationMs)
        {
            return static_cast<uint64_t>(SAMPLE_RATE) * durationMs / 1000;
        }
    }

    SDLAudioBackend::~SDLAudioBackend()
    {
        shutdown();
    }

    bool SDLAudioBackend::initialize()
    {
        if (device_ != 0)
        {
            return true;
        }

        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        {
            return false;
        }

        SDL_AudioSpec desired{};
        desired.freq = SAMPLE_RATE;
        desired.format = AUDIO_S16SYS;
        desired.channels = 1;
        desired.samples = 1024;

        device_ = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
        if (device_ == 0)
        {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
            return false;
        }

        SDL_PauseAudioDevice(device_, 0);
        return true;
    }

    void SDLAudioBackend::shutdown()
    {
        if (device_ != 0)
        {
            SDL_CloseAudioDevice(device_);
            device_ = 0;
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
    }

    bool SDLAudioBackend::play(const SoundDefinition& sound, std::string& error)
    {
        if (device_ == 0)
        {
            error = "SDL audio is not initialized.";
            return false;
        }

        uint64_t sampleCount = 0;
        for (const SoundSegment& segment : sound)
        {
            const uint64_t segmentSampleCount = getSampleCount(segment.durationMs);
            if (segmentSampleCount > MAX_SAMPLE_COUNT - sampleCount)
            {
                error = "sound is too long.";
                return false;
            }
            sampleCount += segmentSampleCount;

            if (segment.type == SoundSegmentType::Tone &&
                (!std::isfinite(segment.startFrequencyHz) ||
                 !std::isfinite(segment.endFrequencyHz) ||
                 segment.startFrequencyHz <= 0.0f ||
                 segment.endFrequencyHz <= 0.0f ||
                 segment.startFrequencyHz >= SAMPLE_RATE / 2.0f ||
                 segment.endFrequencyHz >= SAMPLE_RATE / 2.0f))
            {
                error = "tone frequency is out of range.";
                return false;
            }
        }

        std::vector<int16_t> samples;
        samples.reserve(static_cast<std::size_t>(sampleCount));

        for (const SoundSegment& segment : sound)
        {
            if (segment.type == SoundSegmentType::Pause)
            {
                samples.insert(
                    samples.end(),
                    static_cast<std::size_t>(getSampleCount(segment.durationMs)),
                    0
                );
            }
            else if (!generateTone(
                         samples,
                         segment.startFrequencyHz,
                         segment.endFrequencyHz,
                         segment.durationMs
                     ))
            {
                error = "sample buffer is too large.";
                return false;
            }
        }

        if (samples.empty())
        {
            return true;
        }

        SDL_ClearQueuedAudio(device_);
        if (SDL_QueueAudio(
                device_,
                samples.data(),
                static_cast<Uint32>(samples.size() * sizeof(int16_t))
            ) != 0)
        {
            error = std::string("SDL failed to queue audio: ") + SDL_GetError();
            return false;
        }

        return true;
    }

    bool SDLAudioBackend::generateTone(
        std::vector<int16_t>& samples,
        float startFrequencyHz,
        float endFrequencyHz,
        uint32_t durationMs
    )
    {
        const uint64_t sampleCount = getSampleCount(durationMs);
        if (sampleCount > samples.max_size() - samples.size())
        {
            return false;
        }

        constexpr double amplitude = 0.24;
        const uint64_t fadeSamples = SAMPLE_RATE / 200;

        for (uint64_t i = 0; i < sampleCount; ++i)
        {
            const double progress = sampleCount > 1
                ? static_cast<double>(i) / static_cast<double>(sampleCount - 1)
                : 1.0;
            const double frequency =
                startFrequencyHz + (endFrequencyHz - startFrequencyHz) * progress;
            phase_ += 2.0 * PI * frequency / SAMPLE_RATE;
            if (phase_ >= 2.0 * PI)
            {
                phase_ = std::fmod(phase_, 2.0 * PI);
            }

            double envelope = 1.0;
            if (i < fadeSamples)
            {
                envelope = static_cast<double>(i) / fadeSamples;
            }
            const uint64_t samplesUntilEnd = sampleCount - 1 - i;
            if (samplesUntilEnd < fadeSamples)
            {
                envelope = std::min(
                    envelope,
                    static_cast<double>(samplesUntilEnd) / fadeSamples
                );
            }

            const double tremolo = 1.0 + 0.025 * std::sin(2.0 * PI * 12.0 * progress);
            const double waveform = std::sin(phase_) + 0.12 * std::sin(2.0 * phase_);
            const double value = amplitude * envelope * tremolo * waveform;
            samples.push_back(static_cast<int16_t>(value * std::numeric_limits<int16_t>::max()));
        }

        return true;
    }
}
