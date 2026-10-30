#include "AnimationCatalog.h"
#include "Poses.h"
namespace {
using namespace desktoprobot;
constexpr DrawCommand tired[] = {roundedRectangle(18,22,32,12,6), roundedRectangle(78,22,32,12,6), line(54,51,74,51)};
constexpr DrawCommand surprised[] = {circle(40,25,13), circle(88,25,13), circle(64,51,6)};
constexpr DrawCommand sad[] = {
 line(22,19,46,26), line(82,26,106,19), roundedRectangle(25,28,18,6,2), roundedRectangle(85,28,18,6,2),
 line(54,43,60,39), line(60,39,68,39), line(68,39,74,43)
};
constexpr Frame idleFrames[] = {poses::frame(1000,poses::idle)};
constexpr Frame sleepFrames[] = {poses::frame(1000,poses::closed)};
constexpr Frame tiredFrames[] = {poses::frame(1000,tired)};
constexpr Frame surpriseFrames[] = {poses::frame(900,surprised)};
constexpr Frame errorFrames[] = {poses::frame(1000,sad)};
constexpr Animation idle = {"IDLE",idleFrames,1,true}, sleep = {"SLEEP",sleepFrames,1,true},
 tiredAnim = {"TIRED",tiredFrames,1,true}, surprise = {"SURPRISED",surpriseFrames,1,false}, error = {"ERROR_FACE",errorFrames,1,true};
}
const desktoprobot::Animation& getAnimation(AnimationId id) {
 switch(id) {
 case AnimationId::BLINK: return blinkAnimation;
 case AnimationId::ANGRY: return angryAnimation;
 case AnimationId::SMILE: return smileAnimation;
 case AnimationId::YAWN: return yawnAnimation;
 case AnimationId::SLEEP: return sleep;
 case AnimationId::TIRED: return tiredAnim;
 case AnimationId::SURPRISED: return surprise;
 case AnimationId::ERROR_FACE: return error;
 default: return idle;
 }
}
