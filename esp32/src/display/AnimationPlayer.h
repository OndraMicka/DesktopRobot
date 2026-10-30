#pragma once

#include "Animation.h"

namespace desktoprobot
{
    class AnimationPlayer
    {
    public:
        AnimationPlayer();

        void play(const Animation& animation, std::size_t frameLimit = 0);
        const Frame& frame() const { return getCurrentFrame(); }

        void setLoopAnimation(const Animation& animation);
        void playOneShot(const Animation& animation);
        void update(uint32_t deltaMs);
        const Frame& getCurrentFrame() const;
        void renderCurrentFrame(Framebuffer& framebuffer) const;
        const Animation* getCurrentAnimation() const;
        bool isFinished() const;

    private:
        const Animation* animation_ = nullptr;
        const Animation* previousLoopAnimation_ = nullptr;
        std::size_t currentFrameIndex_ = 0;
        uint32_t elapsedMs_ = 0;
        bool finished_ = false;
        Animation bounded_{};
    };
}

using AnimationPlayer = desktoprobot::AnimationPlayer;
