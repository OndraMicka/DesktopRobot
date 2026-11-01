#pragma once
#include "TestAnimations.h"

namespace desktoprobot {
// Simulator auditions only: deliberately separate from the firmware catalog.
enum class PreviewId {
    Microsleep, Hmm, Kiss, Sigh, SleepRoll, StartleLeft, StartleUpRight,
    HeavyLids, SleepyNod, SleepTwitch, Count
};
constexpr std::size_t PreviewCount = static_cast<std::size_t>(PreviewId::Count);
struct PreviewDefinition {
    PreviewId id; RobotState state; const char* label; uint32_t durationMs = 6000;
};
const std::array<PreviewDefinition, PreviewCount>& getPreviewDefinitions();
const Animation& getPreviewAnimation(PreviewId id);
}
