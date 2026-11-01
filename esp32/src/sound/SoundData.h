#pragma once
#include <cstddef>
#include <cstdint>

namespace desktoprobot {
enum class SoundEffect : uint8_t {
    Shake1, Shake2, Tap1, WakeUp, FallAsleep, Hold, PowerOn,
    // Comparison auditions; Hold is the selected reaction score (variant 3).
    HoldPreview1, HoldPreview2, HoldPreview3, HoldPreview4, Count
};

// Zero Hz is silence. Immutable score data stays in flash on ESP32.
struct ToneSegment {
    uint16_t frequencyHz;
    uint16_t durationMs;
};
struct SoundData {
    const ToneSegment* segments;
    std::size_t count;
    uint32_t durationMs;
};
const SoundData* getSoundData(SoundEffect effect);
}
