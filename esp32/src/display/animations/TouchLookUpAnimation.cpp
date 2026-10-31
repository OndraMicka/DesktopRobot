#include "TouchLookUpAnimation.h"
#include "TouchAnimationBuilder.h"

namespace desktoprobot {
const Animation& TouchLookUpAnimation::get(RobotState source) {
    // Restore the original complete, slow gaze before the smile in IDLE.
    if (source == RobotState::Idle) return getTestAnimation(TestAnimationId::LookUp);

    using namespace animationdata;
    // Dedicated tired/sleep hold gestures open the eyes slowly before the gaze.
    // End at neutral so the existing Smile joins seamlessly. These poses need
    // only two eyes and a neutral mouth: avoid a full ESP32 AnimationStorage.
    constexpr unsigned steps = 8;
    constexpr unsigned frameCount = 5 * steps + 1;
    auto& storage = touchGazeStorage();
    auto& commands = storage.commands;
    auto& frames = storage.frames;
    auto& animation = storage.animation;
    const std::array<Pose, 6> poses{{
        source == RobotState::Sleep ? eyesClosed : tired,
        neutral, glanceUp, shifted(glanceUp, 0, 2), glanceUp, neutral
    }};
    const std::array<uint32_t, 6> durations{{
        source == RobotState::Sleep ? 1100U : 720U,
        360, 660, 390, 330, 390
    }};
    std::array<DrawCommand, MAX_COMMANDS> scratch;
    unsigned index = 0;
    for (unsigned segment = 0; segment + 1 < poses.size(); ++segment) {
        for (unsigned step = 0; step < steps; ++step) {
            std::size_t count = 0;
            storage.poses[index] = interpolatePose(poses[segment], poses[segment + 1], step, steps);
            drawPose(scratch, count, storage.poses[index]);
            std::copy_n(scratch.begin(), 3, commands[index].begin());
            const uint32_t hold = durations[segment] / 3;
            frames[index] = {step == 0 ? hold : (durations[segment] - hold) / (steps - 1), commands[index].data(), 3};
            ++index;
        }
    }
    std::size_t count = 0;
    storage.poses[index] = poses.back();
    drawPose(scratch, count, poses.back());
    std::copy_n(scratch.begin(), 3, commands[index].begin());
    frames[index] = {durations.back(), commands[index].data(), 3};
    animation = {source == RobotState::Sleep ? "SLEEP TOUCH LOOK UP" : "TIRED TOUCH LOOK UP", frames.data(), frameCount, false};
    return animation;
}
}
