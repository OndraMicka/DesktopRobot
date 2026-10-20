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
        desired.callback = audioCallback;
        desired.userdata = this;

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

    void SDLAudioBackend::audioCallback(void* userdata, Uint8* stream, int length)
    {
        auto& self = *static_cast<SDLAudioBackend*>(userdata);
        auto* output = reinterpret_cast<int16_t*>(stream);
        const std::size_t count = static_cast<std::size_t>(length) / sizeof(int16_t);
        std::fill(output, output + count, 0);
        std::size_t written = 0;
        while (written < count && !self.samples_.empty())
        {
            if (self.sampleIndex_ == self.samples_.size())
            {
                if (!self.loop_) break;
                self.sampleIndex_ = 0;
            }
            const std::size_t chunk = std::min(count - written, self.samples_.size() - self.sampleIndex_);
            std::copy_n(self.samples_.data() + self.sampleIndex_, chunk, output + written);
            self.sampleIndex_ += chunk;
            written += chunk;
        }
    }

    void SDLAudioBackend::stop()
    {
        if (device_ == 0) return;
        SDL_LockAudioDevice(device_);
        samples_.clear();
        sampleIndex_ = 0;
        loop_ = false;
        SDL_UnlockAudioDevice(device_);
    }

    bool SDLAudioBackend::play(const SoundDefinition& sound, std::string& error, bool loop)
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

        SDL_LockAudioDevice(device_);
        samples_.swap(samples);
        sampleIndex_ = 0;
        loop_ = loop;
        SDL_UnlockAudioDevice(device_);

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

        constexpr double amplitude = 0.18;
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

            // Odd harmonics give the low tones a restrained electronic buzz.
            const double waveform = std::sin(phase_)
                + 0.25 * std::sin(3.0 * phase_)
                + 0.10 * std::sin(5.0 * phase_);
            const double value = amplitude * envelope * waveform;
            samples.push_back(static_cast<int16_t>(value * std::numeric_limits<int16_t>::max()));
        }

        return true;
    }
}
