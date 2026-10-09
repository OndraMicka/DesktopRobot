#pragma once

namespace desktoprobot
{
    class AudioBackend;

    enum class SoundEffect
    {
        Angry,
        Bored,
        Yawn,
        Hum
    };

    class SoundPlayer
    {
    public:
        explicit SoundPlayer(AudioBackend& backend);

        void play(SoundEffect effect);
        void playAngry();
        void playBored();
        void playYawn();
        void playHum();

    private:
        AudioBackend& backend_;
    };
}
