#include "sound/SoundPlayer.h"
#include "sound/AudioBackend.h"
#include <iostream>
#include <string>

namespace desktoprobot {
SoundPlayer::SoundPlayer(AudioBackend& backend) : backend_(backend) {}

void SoundPlayer::play(SoundEffect effect, bool loop) {
    const auto* data = getSoundData(effect);
    if (!data) return;
    // PCM allocation is PC-only. Firmware reads the same immutable score
    // directly and uses hardware PWM instead of synthesizing samples.
    SoundDefinition definition;
    definition.reserve(data->count);
    for (std::size_t i = 0; i < data->count; ++i) {
        const auto& segment = data->segments[i];
        const float hz = segment.frequencyHz;
        definition.push_back({hz ? SoundSegmentType::Tone : SoundSegmentType::Pause,
                              hz, hz, segment.durationMs});
    }
    std::string error;
    if (!backend_.play(definition, error, loop)) {
        backend_.stop();
        std::cerr << "Failed to play robot sound: "
                  << (error.empty() ? "unknown audio backend error" : error) << '\n';
    }
}
void SoundPlayer::stop() { backend_.stop(); }
bool SoundPlayer::isPlaying() const { return backend_.isPlaying(); }
}
