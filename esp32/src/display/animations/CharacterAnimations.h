#pragma once
#include "TestAnimations.h"

namespace desktoprobot {
constexpr bool isCharacterAnimation(TestAnimationId id) {
    return id>=TestAnimationId::Microsleep && id<TestAnimationId::Count;
}
const Animation& getCharacterAnimation(TestAnimationId id);
}
