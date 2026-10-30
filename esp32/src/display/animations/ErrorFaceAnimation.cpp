#include "ErrorFaceAnimation.h"
#include "Poses.h"
namespace desktoprobot {
using namespace desktoprobot;
constexpr DrawCommand sad[] = {
 line(22,19,46,26), line(82,26,106,19), roundedRectangle(25,28,18,6,2), roundedRectangle(85,28,18,6,2),
 line(54,43,60,39), line(60,39,68,39), line(68,39,74,43)
};
constexpr Frame frames[] = {poses::frame(1000,sad)};
constexpr Animation error = {"ERROR_FACE",frames,1,true};
}
const desktoprobot::Animation& desktoprobot::ErrorFaceAnimation::get() { return error; }
