#pragma once

namespace desktoprobot
{
    class AudioBackend;

    enum class SoundEffect
    {
        Happy,
        SuperHappy,
        Cuddly,
        Angry,
        Surprised,
        Sleep,
        Friendly,
        WakeUp,
        Hum,
        Content,
        Bored
    };

    class SoundPlayer
    {
    public:
        explicit SoundPlayer(AudioBackend& backend);

        void play(SoundEffect effect);
        void playHappy();
        void playSuperHappy();
        void playCuddly();
        void playAngry();
        void playSurprised();
        void playSleep();
        void playFriendly();
        void playWakeUp();
        void playHum();
        void playContent();
        void playBored();

    private:
        AudioBackend& backend_;
    };
}
