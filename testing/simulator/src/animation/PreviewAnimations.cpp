#include "PreviewAnimations.h"
#include "../../../../esp32/src/display/animations/AnimationBuilder.h"
#include <initializer_list>

namespace desktoprobot {
namespace {
using namespace animationdata;
constexpr unsigned SAMPLES = 121;
struct Storage {
    std::array<std::array<DrawCommand, MAX_COMMANDS>, SAMPLES> commands{};
    std::array<Frame, SAMPLES> frames{};
    Animation animation{};
};
struct Key { double time; Pose pose; };
Pose expression(Pose p, Mouth mouth, int width=14, int height=12) {
    p.mouth=mouth; p.mouthWidth=width; p.mouthHeight=height; return p;
}
Pose shape(Pose p, int width, int height, int x=0, int y=0, int roll=0) {
    p.eyeWidth=width; p.eyeHeight=height; p.eyeOffsetX=x; p.eyeOffsetY=y; p.eyeRoll=roll; return p;
}
Pose track(double t, std::initializer_list<Key> keys) {
    auto prev=keys.begin();
    for(auto next=prev+1;next!=keys.end();++next) {
        if(t<=next->time) return interpolatePose(prev->pose,next->pose,
            static_cast<int>(100*(t-prev->time)/(next->time-prev->time)),100);
        prev=next;
    }
    return prev->pose;
}
Pose poseAt(PreviewId id,double t) {
    const auto closed=eyesClosed;
    switch(id) {
    case PreviewId::Microsleep:
        return track(t,{{0,tired},{1,shape(almostClosed,32,5,0,3)},
            {2.3,expression(shape(closed,32,3,0,7),Mouth::Open,10,9)},
            {2.7,expression(shape(closed,32,3,0,7),Mouth::Open,10,9)},
            {2.85,expression(shape(neutral,34,34,0,-3),Mouth::Open,12,10)},
            {3.3,neutral},{4.3,halfLid},{5.4,tired},{6,tired}});
    case PreviewId::Hmm: {
        Pose curious=neutral; curious.gazeX=3; curious.gazeY=-2;
        Pose thoughtful=shape(neutral,32,20,0,0,-1);
        thoughtful.rightEyeHeightOffset=6; thoughtful.gazeX=3; thoughtful.gazeY=-2;
        Pose pondering=thoughtful; pondering.eyeOffsetY=1; pondering.gazeX=4;
        return track(t,{{0,neutral},{.25,neutral},{.55,curious},{1.05,thoughtful},
            {1.65,thoughtful},{2.05,pondering},{2.65,pondering},
            {3.15,thoughtful},{3.7,neutral},{4,neutral}});
    }
    case PreviewId::Kiss:
        return track(t,{{0,neutral},{.25,neutral},{.7,shape(neutral,32,22)},
            {1,closed},{1.3,shape(closed,31,3,0,-1)},
            {1.65,shape(closed,31,3,0,-1)},{2.1,closed},
            {2.45,closed},{2.95,shape(neutral,32,22)},{3.65,neutral},{4,neutral}});
    case PreviewId::Sigh:
        return track(t,{{0,tired},{.65,shape(halfLid,32,16,0,-3)},
            {1,expression(shape(closed,32,3,0,-3),Mouth::Open,8,10)},
            {2.4,expression(shape(closed,32,3,0,4),Mouth::Open,15,7)},
            {3.1,shape(closed,32,3,0,4)},{4,tired},{4.5,tired}});
    case PreviewId::SleepRoll:
        return track(t,{{0,closed},{.5,closed},{1.8,shape(closed,29,3,7,3,4)},
            {3.5,shape(closed,29,3,7,3,4)},{4.8,closed},{5.5,closed}});
    case PreviewId::HeavyLids: {
        Pose leftDroop=shape(tired,32,5,0,1); leftDroop.rightEyeHeightOffset=7;
        Pose bothDroop=leftDroop; bothDroop.rightEyeHeightOffset=2;
        const Pose shut=shape(closed,32,3,0,1);
        const Pose effort=shape(tired,32,18,0,-1);
        return track(t,{{0,tired},{.4,tired},{1.25,leftDroop},{1.85,bothDroop},
            {2.05,shut},{2.8,effort},{3.35,effort},
            {4.4,shape(tired,32,15)},{5.6,tired},{6,tired}});
    }
    case PreviewId::SleepyNod:
        return track(t,{{0,tired},{.35,tired},{.95,shape(closed,32,3,0,1)},
            {1.55,shape(closed,32,3,0,4)},{1.75,shape(closed,32,3,0,4)},
            {2.5,shape(tired,32,8,0,2)},{3.55,tired},{4.2,tired}});
    case PreviewId::SleepTwitch:
        // One brief involuntary movement; no opening, smile or repeating shake.
        return track(t,{{0,closed},{1,closed},{1.05,shifted(closed,-2,-1)},
            {1.15,shifted(closed,1,1)},{1.45,shifted(closed,1,0)},
            {2.15,closed},{3.4,closed}});
    case PreviewId::StartleLeft:
    case PreviewId::StartleUpRight: {
        const bool upRight=id==PreviewId::StartleUpRight;
        Pose snap=neutral; snap.gazeX=upRight ? 14:-14;
        snap.gazeY=upRight ? -6:0; snap.eyeHeight=29;
        Pose watching=neutral; watching.gazeX=upRight ? 13:-13;
        watching.gazeY=upRight ? -6:0;
        Pose calm=watching; calm.eyeHeight=26;
        // Eyes lead in a 100 ms saccade; the face stays steady while inspecting.
        return track(t,{{0,neutral},{.2,neutral},{.3,snap},{.45,watching},
            {2.65,watching},{3.15,calm},{3.5,calm},{4.15,neutral},{5,neutral}});
    }
    default: return tired;
    }
}
double envelope(double t,double start,double rise,double fall,double end) {
    const double u=std::clamp(std::min((t-start)/(rise-start),(end-t)/(end-fall)),0.,1.);
    return u*u*(3-2*u);
}
// Dense filled disks keep moving lips solid and rounded at every intermediate frame.
void lips(std::array<DrawCommand,MAX_COMMANDS>& commands,std::size_t& count,
          int x,int y,int width,int bend,int slope=0) {
    if(bend==0 && slope==0) {
        append(commands,count,roundedRectangle(x-width/2,y-2,width,5,2));
        return;
    }
    const int span=width-5;
    for(int i=0;i<=span;++i) {
        const double u=double(i)/span;
        const int curve=int(std::lround(bend*4*u*(1-u)+slope*(u-.5)));
        append(commands,count,circle(x-width/2+2+i,y+curve,2,true));
    }
}
void heart(std::array<DrawCommand,MAX_COMMANDS>& commands,std::size_t& count,int x,int y) {
    // A fixed solid silhouette preserves the cleft, two rounded lobes and point
    // at OLED resolution. Each span joins the next into one filled shape.
    constexpr int starts[]={2,1,0,0,0,1,2,3,4,5,6,7};
    constexpr int widths[]={4,6,16,16,16,14,12,10,8,6,4,2};
    for(int row=0;row<12;++row) {
        append(commands,count,rectangle(x+starts[row],y+row,widths[row],1));
        if(row<2) append(commands,count,rectangle(x+16-starts[row]-widths[row],y+row,widths[row],1));
    }
}
void drawPreview(std::array<DrawCommand,MAX_COMMANDS>& commands,std::size_t& count,
                 PreviewId id,double t) {
    const auto p=poseAt(id,t);
    // Reuse the original filled eyes and 5-pixel mouth strokes. Special expressions
    // replace individual parts; all new visible geometry is solid, never an outline.
    const bool customEyes=id==PreviewId::SleepRoll && p.eyeRoll!=0;
    if(id==PreviewId::HeavyLids) {
        // Let the upper lids descend while the lower edge stays anchored. The
        // original eye width, rounded corners and five-pixel closed bars remain.
        for(int side:{-1,1}) {
            const int height=std::max(5,p.eyeHeight+(side>0 ? p.rightEyeHeightOffset:0));
            append(commands,count,roundedRectangle(64+side*30-16,
                31+p.eyeOffsetY-height,32,height,std::min(32,height)/2));
        }
        drawMouth(commands,count,Mouth::None,0,p.eyeOffsetY,0,0);
    }
    else if(!customEyes) drawPose(commands,count,p);
    else {
        for(int side:{-1,1}) {
            const int x=64+side*p.eyeSpacing/2+p.eyeOffsetX-p.eyeWidth/2;
            const int y=25+p.eyeOffsetY+side*p.eyeRoll;
            for(int i=0;i<=10;++i)
                append(commands,count,circle(x+2+i*(p.eyeWidth-5)/10,
                    y+(i-5)*p.eyeRoll/10,2,true));
        }
        for(int i=0;i<=8;++i) append(commands,count,circle(56+p.eyeOffsetX+i*2,
            51+p.eyeOffsetY+(i-4)*p.eyeRoll/10,2,true));
    }
    if(id==PreviewId::Hmm) {
        count=2;
        const double thought=envelope(t,.45,1.1,2.65,3.7);
        lips(commands,count,64+p.eyeOffsetX+int(std::lround(4*thought)),
            51+p.eyeOffsetY,20-int(std::lround(3*thought)),0,int(std::lround(-3*thought)));
    }
    if(id==PreviewId::Kiss) {
        count=2;
        const double pucker=envelope(t,.8,1.25,1.65,2.15);
        const double smile=envelope(t,.2,.7,.75,1.15)+envelope(t,1.8,2.3,2.9,3.65);
        const int width=20-int(std::lround(12*pucker))+int(std::lround(5*smile));
        const int height=5+int(std::lround(4*pucker));
        if(pucker>.05) append(commands,count,roundedRectangle(64-width/2,
            51+p.eyeOffsetY-height/2,width,height,std::min(width,height)/2));
        else lips(commands,count,64,51+p.eyeOffsetY-int(std::lround(2*smile)),width,int(std::lround(3*smile)));
        if(t>1.65 && t<2.75) {
            const double u=(t-1.65)/1.1;
            heart(commands,count,74+int(std::lround(24*u)),43-int(std::lround(12*u)));
        }
    }
}
}
const std::array<PreviewDefinition,PreviewCount>& getPreviewDefinitions() {
    static const std::array<PreviewDefinition,PreviewCount> definitions{{
        {PreviewId::Microsleep,RobotState::Tired,"NEW - MICROSLEEP"},
        {PreviewId::Hmm,RobotState::Idle,"NEW - HMM",4000},
        {PreviewId::Kiss,RobotState::Idle,"NEW - KISS",4000},
        {PreviewId::Sigh,RobotState::Tired,"NEW - SIGH",4500},
        {PreviewId::SleepRoll,RobotState::Sleep,"NEW - SLEEP ROLL",5500},
        {PreviewId::StartleLeft,RobotState::Idle,"NEW - STARTLE LEFT",5000},
        {PreviewId::StartleUpRight,RobotState::Idle,"NEW - STARTLE UP RIGHT",5000},
        {PreviewId::HeavyLids,RobotState::Tired,"NEW - HEAVY LIDS",6000},
        {PreviewId::SleepyNod,RobotState::Tired,"NEW - SLEEPY NOD",4200},
        {PreviewId::SleepTwitch,RobotState::Sleep,"NEW - SLEEP TWITCH",3400}
    }};
    return definitions;
}
const Animation& getPreviewAnimation(PreviewId id) {
    // Static storage keeps frame/command pointers stable throughout playback.
    static std::array<Storage,PreviewCount> storage;
    static std::array<bool,PreviewCount> built{};
    const auto index=static_cast<std::size_t>(id);
    auto& s=storage.at(index);
    if(!built[index]) {
        const unsigned frameCount=getPreviewDefinitions()[index].durationMs/50+1;
        for(unsigned i=0;i<frameCount;++i) {
            std::size_t n=0; const double t=i*.05;
            drawPreview(s.commands[i],n,id,t);
            s.frames[i]={i+1==frameCount ? 150U:50U,s.commands[i].data(),n};
        }
        s.animation={getPreviewDefinitions()[index].label,s.frames.data(),frameCount,false};
        built[index]=true;
    }
    return s.animation;
}
}
