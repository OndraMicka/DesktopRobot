#pragma once

namespace desktoprobot
{
    class AudioBackend;

    enum class SoundEffect
    {
        Shake1, Shake2, Tap1,
        WakeUp, FallAsleep, Hold, PowerOn,
        Count
    };

    class SoundPlayer
    {
    public:
        explicit SoundPlayer(AudioBackend& backend);

        void play(SoundEffect effect, bool loop = false);
        void stop();

    private:
        AudioBackend& backend_;
    };
}
