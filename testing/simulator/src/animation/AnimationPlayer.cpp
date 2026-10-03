#include "AnimationPlayer.h"

namespace desktoprobot
{
    AnimationPlayer::AnimationPlayer() = default;

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

        elapsedMs_ += deltaMs;

        while (elapsedMs_ >= animation_->frames[currentFrameIndex_].durationMs)
        {
            elapsedMs_ -= animation_->frames[currentFrameIndex_].durationMs;
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
