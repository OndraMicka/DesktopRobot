#include "AnimationCatalog.h"
#include "Poses.h"
namespace {
constexpr desktoprobot::Frame frames[] = {poses::frame(60,poses::idle), poses::frame(120,poses::closed), poses::frame(60,poses::idle)};
}
const desktoprobot::Animation blinkAnimation = {"BLINK",frames,3,false};
