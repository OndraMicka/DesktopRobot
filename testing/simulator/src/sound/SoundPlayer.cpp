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

    void SoundPlayer::play(SoundEffect effect)
    {
        switch (effect)
        {
        case SoundEffect::Angry:
            playAngry();
            break;
        case SoundEffect::Bored:
            playBored();
            break;
        case SoundEffect::Yawn:
            playYawn();
            break;
        case SoundEffect::Hum:
            playHum();
            break;
        }
    }

    void SoundPlayer::playAngry()
    {
        // Low, clipped servo bursts rather than high-pitched chirps.
        static const SoundDefinition sound = {
            tone(145, 185, 85), pause(35),
            tone(185, 140, 110), pause(45),
            tone(140, 195, 85), pause(30),
            tone(175, 115, 240)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::playBored()
    {
        // A flat acknowledgement followed by a slow electronic grumble.
        static const SoundDefinition sound = {
            tone(165, 165, 150), pause(120),
            tone(155, 110, 480), pause(80),
            tone(110, 105, 180)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::playYawn()
    {
        // A motor winds up, lingers, then runs down with a short tired tail.
        static const SoundDefinition sound = {
            tone(120, 190, 420),
            tone(190, 270, 460),
            tone(270, 245, 280),
            tone(245, 105, 650), pause(90),
            tone(105, 90, 170)
        };
        playSound(backend_, sound);
    }

    void SoundPlayer::playHum()
    {
        // Original low-register motif, played independently of face animations.
        static const SoundDefinition sound = {
            tone(196, 196, 220), pause(70),
            tone(196, 196, 140), pause(70),
            tone(146.83f, 146.83f, 300), pause(100),
            tone(174.61f, 174.61f, 220), pause(70),
            tone(130.81f, 130.81f, 380), pause(160),
            tone(146.83f, 146.83f, 220), pause(70),
            tone(196, 196, 260), pause(90),
            tone(130.81f, 130.81f, 520)
        };
        playSound(backend_, sound);
    }
}
