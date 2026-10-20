#pragma once

namespace desktoprobot
{
    class AudioBackend;

    enum class SoundEffect
    {
        Shake1, Shake2, Shake3, Shake4, Shake5,
        Hold1, Hold2, Hold3, Hold4, Hold5,
        Tap1, Tap2, Tap3, Tap4, Tap5,
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
