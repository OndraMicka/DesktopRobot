#include "CharacterAnimations.h"

namespace desktoprobot {
namespace {
#include "CharacterAnimationData.inc"
}
const Animation& getCharacterAnimation(TestAnimationId id) {
    switch(id) {
    case TestAnimationId::Microsleep: return MicrosleepAnimation;
    case TestAnimationId::Hmm: return HmmAnimation;
    case TestAnimationId::Kiss: return KissAnimation;
    case TestAnimationId::Sigh: return SighAnimation;
    case TestAnimationId::SleepRoll: return SleepRollAnimation;
    case TestAnimationId::StartleLeft: return StartleLeftAnimation;
    case TestAnimationId::StartleUpRight: return StartleUpRightAnimation;
    case TestAnimationId::HeavyLids: return HeavyLidsAnimation;
    case TestAnimationId::SleepyNod: return SleepyNodAnimation;
    case TestAnimationId::SleepTwitch: return SleepTwitchAnimation;
    default: return MicrosleepAnimation;
    }
}
}
