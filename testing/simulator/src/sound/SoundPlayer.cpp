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

        // Short repeating shake phrases and melodic hold auditions.
        // Tap auditions, including the selected TAP 1, stay unchanged.
        const std::array<SoundDefinition, static_cast<std::size_t>(SoundEffect::Count)> sounds = {{
            // SHAKE 1: steady alternating pips with equal spacing.
            { beep(659, 80), pause(50), beep(988, 80), pause(50) },
            // SHAKE 2: two quick high pips answered by a low one.
            { beep(988, 55), pause(35), beep(988, 55), pause(35), beep(659, 110), pause(90) },
            // SHAKE 3: a repeating descending three-note warning.
            { beep(880, 90), pause(40), beep(740, 90), pause(40), beep(587, 90), pause(70) },
            // SHAKE 4: bouncing low/high pairs with a syncopated gap.
            { beep(523, 70), pause(35), beep(784, 100), pause(70),
              beep(587, 70), pause(35), beep(880, 100), pause(70) },
            // SHAKE 5: an even four-note zigzag.
            { beep(784, 65), pause(45), beep(659, 65), pause(45),
              beep(988, 65), pause(45), beep(740, 65), pause(45) },

            // HOLD 1: the selected tap's rising fifth followed by a warm ending.
            { beep(659, 90), pause(45), beep(988, 130), pause(75), beep(784, 260) },
            // HOLD 2: a soft three-note major arpeggio in G.
            { beep(392, 140), pause(40), beep(494, 140), pause(40), beep(587, 280) },
            // HOLD 3: a light two-part greeting.
            { beep(587, 80), pause(40), beep(880, 110), pause(110),
              beep(740, 120), pause(40), beep(988, 230) },
            // HOLD 4: a lilting rise with a repeated finishing note.
            { beep(523, 110), pause(45), beep(659, 110), pause(45),
              beep(784, 90), pause(55), beep(784, 240) },
            // HOLD 5: an unhurried melodic arch returning home.
            { beep(659, 150), pause(55), beep(831, 150), pause(55),
              beep(988, 200), pause(80), beep(659, 280) },

            // TAP 1: a slightly lower rising fifth.
            { beep(659, 65), pause(45), beep(988, 100) },
            // TAP 2: a crisp rising octave.
            { beep(523, 70), pause(55), beep(1047, 90) },
            // TAP 3: a gentler, slower rising major third.
            { beep(784, 85), pause(65), beep(988, 120) },
            // TAP 4: two repeated pips, the second held longer.
            { beep(880, 55), pause(55), beep(880, 115) },
            // TAP 5: a tiny three-note major arpeggio.
            { beep(659, 55), pause(35), beep(831, 55), pause(35), beep(988, 100) }
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
