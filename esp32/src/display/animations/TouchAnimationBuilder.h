#pragma once
#include "AnimationBuilder.h"
namespace desktoprobot {
template<unsigned N> struct TouchPoseStorage {
 std::array<std::array<DrawCommand,3>,N> commands;
 std::array<Frame,N> frames;
 std::array<animationdata::Pose,N> poses;
 Animation animation;
};
inline TouchPoseStorage<9>& touchWakeStorage() { static TouchPoseStorage<9> storage; return storage; }
inline TouchPoseStorage<41>& touchGazeStorage() { static TouchPoseStorage<41> storage; return storage; }
inline const Animation& buildTouchPose(const animationdata::Pose& start,const animationdata::Pose& end,unsigned duration,const char* label) {
 using namespace animationdata;
 auto& storage=touchWakeStorage();
 auto& commands=storage.commands; auto& frames=storage.frames; auto& animation=storage.animation;
 std::array<DrawCommand,MAX_COMMANDS> scratch;
 for(unsigned i=0;i<9;++i) {
  storage.poses[i]=interpolatePose(start,end,i,8);
  std::size_t count=0; drawPose(scratch,count,storage.poses[i]);
  std::copy_n(scratch.begin(),3,commands[i].begin());
  frames[i]={i==8 ? (duration-8*(duration/9)) : (duration/9),commands[i].data(),3};
 }
 animation={label,frames.data(),9,false};
 return animation;
}
inline const Animation* tryTouchReturn(const Frame& frame) {
 for(unsigned i=0;i<touchWakeStorage().animation.frameCount;++i) {
  if(&frame==&touchWakeStorage().frames[i]) {
   const auto start=touchWakeStorage().poses[i];
   return &buildTouchPose(start,animationdata::tired,360,"TOUCH RETURN TIRED");
  }
 }
 for(unsigned i=0;i<touchGazeStorage().animation.frameCount;++i) {
  if(&frame==&touchGazeStorage().frames[i]) {
   const auto start=touchGazeStorage().poses[i];
   return &buildTouchPose(start,animationdata::tired,360,"TOUCH RETURN TIRED");
  }
 }
 return nullptr;
}
}
