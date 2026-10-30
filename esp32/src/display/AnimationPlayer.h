#pragma once
#include "SharedGraphics.h"
class AnimationPlayer {
public:
 void play(const desktoprobot::Animation& animation, uint32_t nowMs);
 bool update(uint32_t nowMs);
 const desktoprobot::Frame& frame() const;
 bool isFinished() const { return finished_; }
private:
 const desktoprobot::Animation* animation_ = nullptr;
 std::size_t index_ = 0;
 uint32_t frameStart_ = 0;
 bool finished_ = false;
};
