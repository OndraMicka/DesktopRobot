#include "AnimationPlayer.h"

namespace desktoprobot
{
    AnimationPlayer::AnimationPlayer() = default;

    void AnimationPlayer::play(const Animation& animation, std::size_t frameLimit)
    {
        previousLoopAnimation_ = nullptr;
        animation_ = &animation;
        if (frameLimit && frameLimit <= animation.frameCount) {
            bounded_ = animation;
            bounded_.frameCount = frameLimit;
            bounded_.loop = false;
            animation_ = &bounded_;
        }
        currentFrameIndex_ = 0;
        elapsedMs_ = 0;
        finished_ = animation_->frameCount == 0;
    }

    void AnimationPlayer::setLoopAnimation(const Animation& animation)
    {
        animation_ = &animation;
        previousLoopAnimation_ = &animation;
        currentFrameIndex_ = 0;
        elapsedMs_ = 0;
        finished_ = false;
    }

    void AnimationPlayer::playOneShot(const Animation& animation)
    {
        if (animation_ != nullptr && animation_->loop)
        {
            previousLoopAnimation_ = animation_;
        }
        else if (previousLoopAnimation_ == nullptr)
        {
            previousLoopAnimation_ = &animation;
        }

        animation_ = &animation;
        currentFrameIndex_ = 0;
        elapsedMs_ = 0;
        finished_ = false;
    }

    void AnimationPlayer::update(uint32_t deltaMs)
    {
        if (animation_ == nullptr || animation_->frameCount == 0)
        {
            return;
        }

        if (finished_)
        {
            return;
        }

        if (animation_->loop) {
            uint32_t cycle = 0;
            for (std::size_t i = 0; i < animation_->frameCount; ++i) cycle += animation_->frames[i].durationMs;
            if (cycle) deltaMs %= cycle;
        }
        uint64_t remaining = uint64_t(elapsedMs_) + deltaMs;
        elapsedMs_ = 0;

        while (remaining >= animation_->frames[currentFrameIndex_].durationMs)
        {
            if (!animation_->frames[currentFrameIndex_].durationMs) { finished_ = true; return; }
            remaining -= animation_->frames[currentFrameIndex_].durationMs;
            ++currentFrameIndex_;

            if (currentFrameIndex_ >= animation_->frameCount)
            {
                if (animation_->loop)
                {
                    currentFrameIndex_ = 0;
                    continue;
                }

                currentFrameIndex_ = animation_->frameCount - 1;
                finished_ = true;

                if (previousLoopAnimation_ != nullptr)
                {
                    animation_ = previousLoopAnimation_;
                    currentFrameIndex_ = 0;
                    elapsedMs_ = 0;
                    finished_ = false;
                }

                return;
            }
        }
        elapsedMs_ = static_cast<uint32_t>(remaining);
    }

    const Frame& AnimationPlayer::getCurrentFrame() const
    {
        static const Frame defaultFrame = { 100, nullptr, 0 };

        if (animation_ == nullptr || animation_->frameCount == 0)
        {
            return defaultFrame;
        }

        return animation_->frames[currentFrameIndex_];
    }

    void AnimationPlayer::renderCurrentFrame(Framebuffer& framebuffer) const
    {
        framebuffer.clear();
        drawFrame(getCurrentFrame(), framebuffer);
    }

    const Animation* AnimationPlayer::getCurrentAnimation() const
    {
        return animation_;
    }

    bool AnimationPlayer::isFinished() const
    {
        return finished_;
    }
}
