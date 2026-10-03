#include "sound/SoundPlayer.h"

#include "sound/AudioBackend.h"

#include <iostream>
#include <string>

namespace desktoprobot
{
    namespace
    {
        constexpr SoundSegment tone(float startFrequencyHz, float endFrequencyHz, uint32_t durationMs)
        {
            return { SoundSegmentType::Tone, startFrequencyHz, endFrequencyHz, durationMs };
        }

        constexpr SoundSegment pause(uint32_t durationMs)
        {
            return { SoundSegmentType::Pause, 0.0f, 0.0f, durationMs };
        }

        void playSound(AudioBackend& backend, const SoundDefinition& sound)
        {
            std::string error;
            if (!backend.play(sound, error))
            {
                std::cerr << "Failed to play robot sound: "
                          << (error.empty() ? "unknown audio backend error" : error)
                          << '\n';
            }
        }
    }

    SoundPlayer::SoundPlayer(AudioBackend& backend)
        : backend_(backend)
    {
    }

    void SoundPlayer::playHappy()
    {
        static const SoundDefinition sound = {
            tone(400, 425, 35), tone(425, 390, 30), tone(390, 445, 35), tone(445, 470, 45),
            pause(25),
            tone(470, 620, 220),
            pause(30),
            tone(620, 800, 180),
            pause(25),
            tone(800, 680, 130),
            pause(25),
            tone(680, 920, 250),
            pause(30),
            tone(920, 1050, 350)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::play(SoundEffect effect)
    {
        switch (effect)
        {
        case SoundEffect::Happy:
            playHappy();
            break;
        case SoundEffect::SuperHappy:
            playSuperHappy();
            break;
        case SoundEffect::Cuddly:
            playCuddly();
            break;
        case SoundEffect::Angry:
            playAngry();
            break;
        case SoundEffect::Surprised:
            playSurprised();
            break;
        case SoundEffect::Sleep:
            playSleep();
            break;
        case SoundEffect::Friendly:
            playFriendly();
            break;
        case SoundEffect::WakeUp:
            playWakeUp();
            break;
        case SoundEffect::Hum:
            playHum();
            break;
        case SoundEffect::Content:
            playContent();
            break;
        case SoundEffect::Bored:
            playBored();
            break;
        }
    }

    void SoundPlayer::playSuperHappy()
    {
        static const SoundDefinition sound = {
            tone(520, 760, 100), tone(760, 650, 65), tone(650, 880, 110),
            pause(25),
            tone(880, 1180, 160), tone(1180, 980, 70), tone(980, 1320, 180)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::playCuddly()
    {
        static const SoundDefinition sound = {
            tone(430, 490, 170),
            pause(35),
            tone(490, 440, 140),
            pause(45),
            tone(440, 540, 210),
            pause(35),
            tone(540, 500, 260)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::playAngry()
    {
        static const SoundDefinition sound = {
            tone(260, 340, 55), tone(340, 250, 45), tone(250, 350, 55), tone(350, 240, 50),
            pause(25),
            tone(300, 390, 95), tone(390, 290, 75), tone(290, 370, 95)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::playSurprised()
    {
        static const SoundDefinition sound = {
            tone(300, 1050, 180),
            pause(45),
            tone(1050, 720, 150)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::playSleep()
    {
        static const SoundDefinition sound = {
            tone(520, 430, 240),
            pause(70),
            tone(430, 350, 300),
            pause(90),
            tone(350, 260, 420)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::playFriendly()
    {
        static const SoundDefinition sound = {
            tone(520, 660, 180),
            pause(35),
            tone(660, 780, 180),
            pause(35),
            tone(780, 720, 260)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::playWakeUp()
    {
        static const SoundDefinition sound = {
            tone(260, 390, 280),
            pause(45),
            tone(390, 620, 220),
            pause(35),
            tone(620, 740, 180)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::playHum()
    {
        static const SoundDefinition sound = {
            tone(390, 430, 130),
            pause(20),
            tone(430, 470, 170),
            pause(25),
            tone(470, 420, 130),
            pause(20),
            tone(420, 520, 210)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::playContent()
    {
        static const SoundDefinition sound = {
            tone(480, 560, 240),
            pause(30),
            tone(560, 620, 300)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::playBored()
    {
        static const SoundDefinition sound = {
            tone(300, 270, 130),
            pause(25),
            tone(270, 245, 180)
        };
        playSound(backend_, sound);
    }
}
