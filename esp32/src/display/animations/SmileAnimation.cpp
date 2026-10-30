#include "AnimationCatalog.h"
#include "Poses.h"
namespace {
using namespace desktoprobot;
constexpr DrawCommand smile[] = {
 line(21,28,28,22), line(28,22,40,22), line(40,22,47,28),
 line(81,28,88,22), line(88,22,100,22), line(100,22,107,28),
 line(48,46,56,52), line(56,52,72,52), line(72,52,80,46)
};
constexpr Frame frames[] = {poses::frame(100,poses::idle), poses::frame(1200,smile), poses::frame(150,poses::idle)};
}
const desktoprobot::Animation smileAnimation = {"SMILE",frames,3,false};
