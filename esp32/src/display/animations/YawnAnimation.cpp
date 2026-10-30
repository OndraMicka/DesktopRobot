#include "AnimationCatalog.h"
#include "Poses.h"
namespace {
using namespace desktoprobot;
constexpr DrawCommand yawn[] = {roundedRectangle(18,24,32,5,2), roundedRectangle(78,24,32,5,2), roundedRectangle(57,41,14,19,7)};
constexpr Frame frames[] = {poses::frame(200,poses::closed), poses::frame(1100,yawn), poses::frame(200,poses::idle)};
}
const desktoprobot::Animation yawnAnimation = {"YAWN",frames,3,false};
