#pragma once
#include "AnimationBuilder.h"
namespace desktoprobot {
inline const Animation& buildTouchPose(const animationdata::Pose& start,const animationdata::Pose& end,unsigned duration,const char* label) {
 using namespace animationdata;
 static std::array<std::array<DrawCommand,3>,9> commands;
 static std::array<Frame,9> frames;
 static Animation animation;
 std::array<DrawCommand,MAX_COMMANDS> scratch;
 for(unsigned i=0;i<9;++i) {
  std::size_t count=0; drawPose(scratch,count,interpolatePose(start,end,i,8));
  std::copy_n(scratch.begin(),3,commands[i].begin());
  frames[i]={i==8 ? (duration-8*(duration/9)) : (duration/9),commands[i].data(),3};
 }
 animation={label,frames.data(),9,false};
 return animation;
}
}
