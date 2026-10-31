#include "sound/SoundPlayer.h"
#include "sound/AudioBackend.h"

#include <array>
#include <iostream>
#include <string>

namespace desktoprobot
{
    namespace
    {
        constexpr SoundSegment beep(float frequencyHz, uint32_t durationMs)
        {
            return { SoundSegmentType::Tone, frequencyHz, frequencyHz, durationMs };
        }

        constexpr SoundSegment pause(uint32_t durationMs)
        {
            return { SoundSegmentType::Pause, 0.0f, 0.0f, durationMs };
        }

        // Standalone buzzer auditions. SHAKE 1/2 and TAP 1 retain their exact
        // original notes and timing; the four new sounds are UI previews only.
        const std::array<SoundDefinition, static_cast<std::size_t>(SoundEffect::Count)> sounds = {{
            // SHAKE 1: steady alternating pips with equal spacing.
            { beep(659, 80), pause(50), beep(988, 80), pause(50) },
            // SHAKE 2: two quick high pips answered by a low one.
            { beep(988, 55), pause(35), beep(988, 55), pause(35), beep(659, 110), pause(90) },
            // TAP 1: a slightly lower rising fifth.
            { beep(659, 65), pause(45), beep(988, 100) },
            // WAKE UP: a bright rising greeting, waking into an open fifth.
            { beep(440, 100), pause(45), beep(659, 120), pause(45), beep(880, 210) },
            // FALL ASLEEP: an unhurried descending phrase with a low ending.
            { beep(659, 150), pause(80), beep(494, 200), pause(110), beep(330, 300) },
            // HOLD: two friendly little pips followed by a sustained answer.
            { beep(740, 85), pause(45), beep(988, 100), pause(90), beep(880, 270) },
            // POWER ON: a short, distinct startup signature after plugging in.
            { beep(330, 70), pause(35), beep(440, 70), pause(35),
              beep(659, 110), pause(65), beep(880, 200) }
        }};
    }

    SoundPlayer::SoundPlayer(AudioBackend& backend)
        : backend_(backend)
    {
    }

    void SoundPlayer::play(SoundEffect effect, bool loop)
    {
        const auto index = static_cast<std::size_t>(effect);
        if (index >= sounds.size()) return;

        std::string error;
        if (!backend_.play(sounds[index], error, loop))
        {
            std::cerr << "Failed to play robot sound: "
                      << (error.empty() ? "unknown audio backend error" : error) << '\n';
        }
    }

    void SoundPlayer::stop()
    {
        backend_.stop();
    }
}
