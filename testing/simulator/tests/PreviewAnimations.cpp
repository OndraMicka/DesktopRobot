#include "animation/PreviewAnimations.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

int main(int argc,char** argv) {
    using namespace desktoprobot;
    for(const auto& definition:getPreviewDefinitions()) {
        const auto& animation=getPreviewAnimation(definition.id);
        const auto robotId=static_cast<TestAnimationId>(
            static_cast<unsigned>(TestAnimationId::Microsleep)+static_cast<unsigned>(definition.id));
        const auto& robotAnimation=getTestAnimation(robotId);
        assert(robotAnimation.frameCount==animation.frameCount && robotAnimation.loop==animation.loop);
        assert(!animation.loop && animation.frameCount>1);
        Framebuffer base,first,last;
        drawFrame(getStateAnimation(definition.state).frames[0],base);
        drawFrame(animation.frames[0],first);
        drawFrame(animation.frames[animation.frameCount-1],last);
        assert(first.data()==base.data() && last.data()==base.data());
        std::set<std::array<uint8_t,Framebuffer::BufferSize>> frames;
        uint32_t duration=0;
        for(std::size_t i=0;i<animation.frameCount;++i) {
            const auto& frame=animation.frames[i];
            assert(frame.durationMs && frame.commandCount && frame.commandCount<80);
            for(std::size_t j=0;j<frame.commandCount;++j) {
                const auto& command=frame.commands[j];
                assert(command.type!=DrawCommandType::Line && command.type!=DrawCommandType::Pixel);
                assert(command.filled);
                if(command.type==DrawCommandType::Circle) assert(command.x2>=2);
            }
            duration+=frame.durationMs;
            Framebuffer image; drawFrame(frame,image); frames.insert(image.data());
            Framebuffer robotImage; drawFrame(robotAnimation.frames[i],robotImage);
            assert(robotImage.data()==image.data());
            assert(robotAnimation.frames[i].durationMs==frame.durationMs);
        }
        assert(frames.size()>(definition.id==PreviewId::SleepTwitch ? 3U:6U));
        assert(duration>=3000 && duration<=7000);
        if(argc>1) {
            std::ofstream out(std::string(argv[1])+"/"+std::to_string(static_cast<int>(definition.id))+".pbm");
            out<<"P1\n"<<128*8<<" 64\n";
            for(int y=0;y<64;++y) {
                for(int slot=0;slot<8;++slot) {
                    auto sample=slot*(animation.frameCount-1)/7;
                    if(definition.id==PreviewId::HeavyLids) sample=std::array<unsigned,8>{0,12,25,37,41,56,80,120}[slot];
                    if(definition.id==PreviewId::SleepyNod) sample=std::array<unsigned,8>{0,12,19,31,40,50,65,84}[slot];
                    if(definition.id==PreviewId::SleepTwitch) sample=std::array<unsigned,8>{0,20,21,22,23,29,43,68}[slot];
                    Framebuffer image; drawFrame(animation.frames[sample],image);
                    for(int x=0;x<128;++x) out<<(image.getPixel(x,y)?"1 ":"0 ");
                }
                out<<'\n';
            }
        }
    }
    assert(getPreviewDefinitions().size()==10);
    const auto& lids=getPreviewAnimation(PreviewId::HeavyLids);
    // One lid gives in first; both then reopen together and settle to TIRED.
    assert(lids.frames[25].commands[0].y2<lids.frames[25].commands[1].y2-4);
    assert(lids.frames[41].commands[0].y2==5 && lids.frames[41].commands[1].y2==5);
    assert(lids.frames[56].commands[0].y2>lids.frames[0].commands[0].y2);
    assert(lids.frames[56].commands[0].y2==lids.frames[56].commands[1].y2);
    const auto& nod=getPreviewAnimation(PreviewId::SleepyNod);
    assert(nod.frames[31].commands[2].y1>nod.frames[0].commands[2].y1);
    assert(nod.frames[31].commands[0].y2==5);
    assert(nod.frames[50].commands[0].y2>5 && nod.frames[50].commands[0].y2<12);
    int nodDirection=0,turns=0,previousY=nod.frames[0].commands[2].y1;
    for(std::size_t i=1;i<nod.frameCount;++i) {
        const int y=nod.frames[i].commands[2].y1;
        const int direction=(y>previousY)-(y<previousY);
        if(direction && nodDirection && direction!=nodDirection) ++turns;
        if(direction) nodDirection=direction;
        previousY=y;
    }
    assert(turns==1); // One nod, without a second bob or overshoot.
    const auto& twitch=getPreviewAnimation(PreviewId::SleepTwitch);
    for(std::size_t i=0;i<twitch.frameCount;++i) {
        const auto& frame=twitch.frames[i];
        assert(frame.commands[0].y2==5 && frame.commands[1].y2==5);
        assert(std::abs(frame.commands[2].x1-54)<=2);
        assert(std::abs(frame.commands[2].y1-49)<=1);
        if(i<=20 || i>=43) {
            Framebuffer image,base;
            drawFrame(frame,image); drawFrame(twitch.frames[0],base);
            assert(image.data()==base.data());
        }
    }
    for(const auto* animation:{&lids,&nod,&twitch}) {
        for(std::size_t i=0;i<animation->frameCount;++i) {
            const auto& frame=animation->frames[i];
            assert(frame.commandCount==3); // Two solid eyes and a neutral mouth.
            const auto& mouth=frame.commands[2];
            assert(mouth.type==DrawCommandType::RoundedRectangle && mouth.x2==20 && mouth.y2==5);
            for(int eye=0;eye<2;++eye) {
                const auto& current=frame.commands[eye];
                assert(current.x2==32 && current.y2>=5 && current.y2<=18);
                if(i) {
                    const auto& previous=animation->frames[i-1].commands[eye];
                    assert(std::abs(current.x1-previous.x1)<=2);
                    assert(std::abs(current.y1-previous.y1)<=3);
                    assert(std::abs(current.y2-previous.y2)<=3);
                }
            }
        }
    }
    // A fast sideways glance, steady inspection and a slower return.
    const auto& startle=getPreviewAnimation(PreviewId::StartleLeft);
    const auto eyeCenter=[&](unsigned sample) {
        const auto& eye=startle.frames[sample].commands[0];
        return eye.x1+eye.x2/2;
    };
    assert(eyeCenter(6)<eyeCenter(4)-12);
    for(std::size_t i=0;i<startle.frameCount;++i) {
        const auto& eye=startle.frames[i].commands[0];
        assert(eye.x2==32 && eye.y2>=26 && eye.y2<=29);
    }
    assert(eyeCenter(24)==eyeCenter(50));
    assert(eyeCenter(66)<24);
    assert(eyeCenter(88)>eyeCenter(66)+10);
    assert(startle.frameCount<getPreviewAnimation(PreviewId::Microsleep).frameCount);
    const auto& upRight=getPreviewAnimation(PreviewId::StartleUpRight);
    assert(upRight.frameCount==startle.frameCount);
    for(std::size_t i=0;i<startle.frameCount;++i) {
        // Both variants retain the original neutral mouth for their entire run.
        for(const auto* animation:{&startle,&upRight}) {
            const auto& frame=animation->frames[i];
            assert(frame.commandCount==3);
            const auto& mouth=frame.commands[2];
            assert(mouth.type==DrawCommandType::RoundedRectangle);
            assert(mouth.x1==54 && mouth.y1==49 && mouth.x2==20 && mouth.y2==5);
        }
        const auto& leftEye=startle.frames[i].commands[0];
        const auto& rightEye=upRight.frames[i].commands[0];
        assert(leftEye.x2==rightEye.x2 && leftEye.y2==rightEye.y2);
        assert(startle.frames[i].durationMs==upRight.frames[i].durationMs);
    }
    const auto& glance=upRight.frames[6].commands[0];
    const auto& origin=upRight.frames[4].commands[0];
    assert(glance.x1>origin.x1+12 && glance.y1<origin.y1-4);
    assert(upRight.frames[24].commands[0].x1==upRight.frames[50].commands[0].x1);
    assert(upRight.frames[24].commands[0].y1==upRight.frames[50].commands[0].y1);
    // The flying heart has a visible top cleft, filled middle and centered tip.
    const auto& kiss=getPreviewAnimation(PreviewId::Kiss);
    const auto& heartTop=kiss.frames[40].commands[3];
    Framebuffer kissImage; drawFrame(kiss.frames[40],kissImage);
    const int hx=heartTop.x1-2, hy=heartTop.y1;
    assert(kissImage.getPixel(hx+3,hy) && kissImage.getPixel(hx+12,hy));
    assert(!kissImage.getPixel(hx+7,hy) && !kissImage.getPixel(hx+8,hy));
    for(int x=0;x<16;++x) assert(kissImage.getPixel(hx+x,hy+4));
    assert(kissImage.getPixel(hx+7,hy+11) && kissImage.getPixel(hx+8,hy+11));
    assert(!kissImage.getPixel(hx+4,hy+11) && !kissImage.getPixel(hx+11,hy+11));
    std::cout<<"PASS: all 10 firmware animations match reviewed previews pixel-for-pixel and duration-for-duration; solid geometry, exact endpoints and gesture phases\n";
}
