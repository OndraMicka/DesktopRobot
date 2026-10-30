#include "AnimationCatalog.h"
#include "Poses.h"
namespace {
using namespace desktoprobot;
constexpr DrawCommand angry[] = {
 roundedRectangle(18,17,32,22,8), roundedRectangle(78,17,32,22,8),
 line(18,13,50,23), line(78,23,110,13),
 line(53,53,60,49), line(60,49,68,49), line(68,49,75,53)
};
constexpr Frame frames[] = {poses::frame(100,poses::idle), poses::frame(950,angry), poses::frame(150,poses::idle)};
}
const desktoprobot::Animation angryAnimation = {"ANGRY",frames,3,false};
