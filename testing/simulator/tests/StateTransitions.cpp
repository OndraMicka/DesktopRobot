#include "animation/TestAnimations.h"
#include "animation/AnimationPlayer.h"

#include <cassert>
#include <iostream>

using namespace desktoprobot;

int main()
{
    int transitions = 0;
    for (auto from : { RobotState::Idle, RobotState::Tired, RobotState::Sleep })
    {
        for (auto to : { RobotState::Idle, RobotState::Tired, RobotState::Sleep })
        {
            const auto* definition = getStateTransition(from, to);
            if (from == to)
            {
                assert(definition == nullptr);
                continue;
            }
            assert(definition != nullptr);
            const auto& animation = getTestAnimation(definition->id);
            assert(!animation.loop && animation.frameCount > 0);
            Framebuffer start, end, expectedStart, expectedEnd;
            drawFrame(animation.frames[0], start);
            drawFrame(animation.frames[animation.frameCount - 1], end);
            drawFrame(getStateAnimation(from).frames[0], expectedStart);
            drawFrame(getStateAnimation(to).frames[0], expectedEnd);
            assert(start.data() == expectedStart.data());
            assert(end.data() == expectedEnd.data());

            uint32_t duration = 0;
            for (std::size_t i = 0; i < animation.frameCount; ++i)
            {
                assert(animation.frames[i].durationMs > 0);
                duration += animation.frames[i].durationMs;
            }
            AnimationPlayer player;
            player.setLoopAnimation(getStateAnimation(to));
            player.playOneShot(animation);
            player.update(duration - 1);
            assert(player.getCurrentAnimation() == &animation);
            player.update(1);
            assert(player.getCurrentAnimation() == &getStateAnimation(to));
            player.update(15000);
            assert(player.getCurrentAnimation() == &getStateAnimation(to));
            ++transitions;
        }
    }
    assert(transitions == 6);
    std::cout << "All six transitions match their source/target faces and finish in the target loop.\n";

    const auto& shake = getTestAnimation(TestAnimationId::Shaking);
    for (std::size_t index = 0; index < shake.frameCount; ++index)
    {
        const auto& recovery = getShakeRecoveryAnimation(shake.frames[index]);
        Framebuffer released, start, end, neutral;
        drawFrame(shake.frames[index], released);
        drawFrame(recovery.frames[0], start);
        drawFrame(recovery.frames[recovery.frameCount - 1], end);
        drawFrame(getStateAnimation(RobotState::Idle).frames[0], neutral);
        assert(released.data() == start.data());
        assert(end.data() == neutral.data());

        uint32_t duration = 0;
        for (std::size_t frame = 0; frame < recovery.frameCount; ++frame)
        {
            assert(recovery.frames[frame].durationMs > 0);
            duration += recovery.frames[frame].durationMs;
        }
        for (auto state : { RobotState::Idle, RobotState::Tired, RobotState::Sleep })
        {
            AnimationPlayer player;
            player.setLoopAnimation(getStateAnimation(state));
            player.playOneShot(recovery);
            player.update(duration);
            assert(player.getCurrentAnimation() == &getStateAnimation(state));
            player.playOneShot(recovery);
            player.update(500);
            player.setLoopAnimation(shake);
            assert(player.getCurrentAnimation() == &shake);
        }
    }
    std::cout << "Every shake release pose joins dizzy seamlessly, recovers, and can be interrupted.\n";
}
