#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace desktoprobot
{
    enum class SoundSegmentType
    {
        Tone,
        Pause
    };

    struct SoundSegment
    {
        SoundSegmentType type;
        float startFrequencyHz;
        float endFrequencyHz;
        uint32_t durationMs;
    };

    using SoundDefinition = std::vector<SoundSegment>;

    class AudioBackend
    {
    public:
        virtual ~AudioBackend() = default;
        virtual bool play(const SoundDefinition& sound, std::string& error, bool loop = false) = 0;
        virtual void stop() = 0;
    };
}
