#include "SoundData.h"

namespace desktoprobot {
namespace {
constexpr ToneSegment shake1[] = {{659,80}, {0,50}, {988,80}, {0,50}};
constexpr ToneSegment shake2[] = {{988,55}, {0,35}, {988,55}, {0,35}, {659,110}, {0,90}};
constexpr ToneSegment tap1[] = {{659,65}, {0,45}, {988,100}};
constexpr ToneSegment wakeUp[] = {{440,100}, {0,45}, {659,120}, {0,45}, {880,210}};
constexpr ToneSegment fallAsleep[] = {{659,150}, {0,80}, {494,200}, {0,110}, {330,300}};
constexpr ToneSegment powerOn[] = {{330,70}, {0,35}, {440,70}, {0,35}, {659,110}, {0,65}, {880,200}};
// Friendly HOLD comparisons. Variant 3 also supplies the selected Hold score.
// 1: a warm rising triad, settling gently on the middle note (940 ms).
constexpr ToneSegment holdPreview1[] = {{523,140}, {0,50}, {659,160}, {0,60}, {784,260}, {0,70}, {659,200}};
// 2: two bright chirps with a higher answer (670 ms).
constexpr ToneSegment holdPreview2[] = {{880,70}, {0,45}, {1175,100}, {0,160}, {880,70}, {0,45}, {1319,180}};
// 3: a low, unhurried three-note greeting (960 ms).
constexpr ToneSegment holdPreview3[] = {{392,220}, {0,65}, {494,220}, {0,75}, {587,380}};
// 4: a playful bounce followed by a high, sustained ending (945 ms).
constexpr ToneSegment holdPreview4[] = {{659,100}, {0,45}, {784,100}, {0,45}, {988,140}, {0,80}, {784,120}, {0,55}, {1047,260}};
template<std::size_t N> constexpr SoundData score(const ToneSegment (&segments)[N]) {
    uint32_t duration = 0;
    for (const auto& segment : segments) duration += segment.durationMs;
    return {segments, N, duration};
}
constexpr SoundData sounds[] = {
    score(shake1), score(shake2), score(tap1), score(wakeUp),
    score(fallAsleep), score(holdPreview3), score(powerOn),
    score(holdPreview1), score(holdPreview2), score(holdPreview3), score(holdPreview4)
};
static_assert(sizeof(ToneSegment) == 4, "Scores must remain compact");
static_assert(sizeof(sounds)/sizeof(sounds[0]) == static_cast<std::size_t>(SoundEffect::Count),
              "Every effect needs a score");
}
const SoundData* getSoundData(SoundEffect effect) {
    const auto index = static_cast<std::size_t>(effect);
    return index < static_cast<std::size_t>(SoundEffect::Count) ? &sounds[index] : nullptr;
}
}
