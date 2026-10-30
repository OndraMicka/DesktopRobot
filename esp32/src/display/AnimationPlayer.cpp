#include "AnimationPlayer.h"
void AnimationPlayer::play(const desktoprobot::Animation& animation, uint32_t nowMs) {
 animation_ = &animation; index_ = 0; frameStart_ = nowMs; finished_ = animation.frameCount == 0;
}
bool AnimationPlayer::update(uint32_t nowMs) {
 if (!animation_ || finished_) return false;
 if (animation_->loop) {
  uint32_t cycle = 0;
  for (std::size_t i=0; i<animation_->frameCount; ++i) cycle += animation_->frames[i].durationMs;
  if (cycle) frameStart_ += ((nowMs-frameStart_)/cycle)*cycle;
 }
 bool changed = false;
 while (nowMs-frameStart_ >= frame().durationMs) {
  uint32_t duration = frame().durationMs;
  if (!duration) { finished_ = true; break; }
  frameStart_ += duration;
  if (index_+1 < animation_->frameCount) ++index_;
  else if (animation_->loop) index_=0;
  else { finished_=true; break; }
  changed=true;
 }
 return changed;
}
const desktoprobot::Frame& AnimationPlayer::frame() const {
 static const desktoprobot::Frame empty = {1,nullptr,0};
 return animation_ && animation_->frameCount ? animation_->frames[index_] : empty;
}
