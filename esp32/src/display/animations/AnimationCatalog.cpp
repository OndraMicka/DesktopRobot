#include "CharacterAnimations.h"
#include "AnimationBuilder.h"
#include "BlinkAnimation.h"
#include "LookLeftAnimation.h"
#include "LookRightAnimation.h"
#include "LookUpAnimation.h"
#include "LookAroundAnimation.h"
#include "ThinkingAnimation.h"
#include "IdleYawnAnimation.h"
#include "ShakingAnimation.h"
#include "AngryAnimation.h"
#include "SmileAnimation.h"
#include "BoredAnimation.h"
#include "ListeningAnimation.h"
#include "RandomCuriosityAnimation.h"
#include "TiredYawnAnimation.h"
#include "SlowBlinkAnimation.h"
#include "SlowEyeMovementAnimation.h"
#include "SleepEyeOpenAnimation.h"
#include "SleepZAnimation.h"
#include "WakeUpAnimation.h"
#include "TiredToSleepAnimation.h"
#include "IdleToTiredAnimation.h"
#include "IdleToSleepAnimation.h"
#include "TiredToIdleAnimation.h"
#include "SleepToTiredAnimation.h"
#include "DizzyAnimation.h"
#include "IdleAnimation.h"
#include "TiredAnimation.h"
#include "SleepAnimation.h"
#include "TouchAnimationBuilder.h"
// CODEX TEST
 #include "TestAnimations.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <new>

namespace desktoprobot
{
    namespace
    {
        using namespace animationdata;
        void initializeAnimations(AnimationStorage& animation, TestAnimationId selected) {
            switch(selected) {
case TestAnimationId::Blink: BlinkAnimation::build(animation); break;
case TestAnimationId::LookLeft: LookLeftAnimation::build(animation); break;
case TestAnimationId::LookRight: LookRightAnimation::build(animation); break;
case TestAnimationId::LookUp: LookUpAnimation::build(animation); break;
case TestAnimationId::LookAround: LookAroundAnimation::build(animation); break;
case TestAnimationId::Thinking: ThinkingAnimation::build(animation); break;
case TestAnimationId::IdleYawn: IdleYawnAnimation::build(animation); break;
case TestAnimationId::Shaking: ShakingAnimation::build(animation); break;
case TestAnimationId::Angry: AngryAnimation::build(animation); break;
case TestAnimationId::Smile: SmileAnimation::build(animation); break;
case TestAnimationId::Bored: BoredAnimation::build(animation); break;
case TestAnimationId::Listening: ListeningAnimation::build(animation); break;
case TestAnimationId::RandomCuriosity: RandomCuriosityAnimation::build(animation); break;
case TestAnimationId::TiredYawn: TiredYawnAnimation::build(animation); break;
case TestAnimationId::SlowBlink: SlowBlinkAnimation::build(animation); break;
case TestAnimationId::SlowEyeMovement: SlowEyeMovementAnimation::build(animation); break;
case TestAnimationId::SleepEyeOpen: SleepEyeOpenAnimation::build(animation); break;
case TestAnimationId::SleepZ: SleepZAnimation::build(animation); break;
case TestAnimationId::WakeUp: WakeUpAnimation::build(animation); break;
case TestAnimationId::TiredToSleep: TiredToSleepAnimation::build(animation); break;
case TestAnimationId::IdleToTired: IdleToTiredAnimation::build(animation); break;
case TestAnimationId::IdleToSleep: IdleToSleepAnimation::build(animation); break;
case TestAnimationId::TiredToIdle: TiredToIdleAnimation::build(animation); break;
case TestAnimationId::SleepToTired: SleepToTiredAnimation::build(animation); break;
case TestAnimationId::Dizzy: DizzyAnimation::build(animation); break;
default: break;
            }
        }

        const std::array<AnimationTestDefinition, static_cast<std::size_t>(TestAnimationId::Count)> animationTests = {{
            { TestAnimationId::Blink, RobotState::Idle, "BLINK" },
            { TestAnimationId::LookLeft, RobotState::Idle, "LOOK LEFT" },
            { TestAnimationId::LookRight, RobotState::Idle, "LOOK RIGHT" },
            { TestAnimationId::LookUp, RobotState::Idle, "LOOK UP" },
            { TestAnimationId::LookAround, RobotState::Idle, "LOOK AROUND" },
            { TestAnimationId::Thinking, RobotState::Idle, "THINKING" },
            { TestAnimationId::IdleYawn, RobotState::Idle, "YAWN" },
            { TestAnimationId::Shaking, RobotState::Idle, "SHAKE - HOLD" },
            { TestAnimationId::Dizzy, RobotState::Idle, "DIZZY" },
            { TestAnimationId::Angry, RobotState::Idle, "ANGRY" },
            { TestAnimationId::Smile, RobotState::Idle, "SMILE" },
            { TestAnimationId::Bored, RobotState::Idle, "BORED" },
            { TestAnimationId::Listening, RobotState::Idle, "LISTENING" },
            { TestAnimationId::RandomCuriosity, RobotState::Idle, "RANDOM CURIOSITY" },
            { TestAnimationId::TiredYawn, RobotState::Tired, "YAWN" },
            { TestAnimationId::SlowBlink, RobotState::Tired, "SLOW BLINK" },
            { TestAnimationId::SlowEyeMovement, RobotState::Tired, "SLOW EYE MOVEMENT" },
            { TestAnimationId::SleepEyeOpen, RobotState::Sleep, "SLEEP EYE OPEN" },
            { TestAnimationId::SleepZ, RobotState::Sleep, "ZZZ" },
            { TestAnimationId::WakeUp, RobotState::Sleep, "WAKE UP - TO IDLE", true, RobotState::Idle },
            { TestAnimationId::TiredToSleep, RobotState::Tired, "TIRED TO SLEEP", true, RobotState::Sleep },
            { TestAnimationId::IdleToTired, RobotState::Idle, "IDLE TO TIRED", true, RobotState::Tired },
            { TestAnimationId::IdleToSleep, RobotState::Idle, "IDLE TO SLEEP", true, RobotState::Sleep },
            { TestAnimationId::TiredToIdle, RobotState::Tired, "TIRED TO IDLE", true, RobotState::Idle },
            { TestAnimationId::SleepToTired, RobotState::Sleep, "SLEEP TO TIRED", true, RobotState::Tired },
            { TestAnimationId::Microsleep, RobotState::Tired, "MICROSLEEP" },
            { TestAnimationId::Hmm, RobotState::Idle, "HMM" },
            { TestAnimationId::Kiss, RobotState::Idle, "KISS" },
            { TestAnimationId::Sigh, RobotState::Tired, "SIGH" },
            { TestAnimationId::SleepRoll, RobotState::Sleep, "SLEEP ROLL" },
            { TestAnimationId::StartleLeft, RobotState::Idle, "STARTLE LEFT" },
            { TestAnimationId::StartleUpRight, RobotState::Idle, "STARTLE UP RIGHT" },
            { TestAnimationId::HeavyLids, RobotState::Tired, "HEAVY LIDS" },
            { TestAnimationId::SleepyNod, RobotState::Tired, "SLEEPY NOD" },
            { TestAnimationId::SleepTwitch, RobotState::Sleep, "SLEEP TWITCH" }
        }};

        void createStateAnimation(AnimationStorage& result, const char* name, const Pose&) {
            if (name[0]=='I') IdleAnimation::build(result);
            else if (name[0]=='T') TiredAnimation::build(result);
            else SleepAnimation::build(result);
        }

#ifdef ROBOT_COMPACT_ANIMATIONS
        // One active catalog buffer, filled in place: no 100 KB stack temporaries.
        // Keep the workspace in BSS. Pose default members otherwise cause the
        // entire mostly-zero buffer to be serialized into the upload image.
        alignas(AnimationStorage) unsigned char activeStorageBytes[sizeof(AnimationStorage)];
        AnimationStorage& activeStorage = *new (activeStorageBytes) AnimationStorage{};
#else
        const std::array<AnimationStorage, static_cast<std::size_t>(TestAnimationId::Count)>& getAnimations()
        {
            static std::array<AnimationStorage, static_cast<std::size_t>(TestAnimationId::Count)> result{};
            static const bool initialized = []() {
                for (std::size_t i = 0; i < result.size(); ++i)
                    initializeAnimations(result[i], static_cast<TestAnimationId>(i));
                return true;
            }();
            (void)initialized;
            return result;
        }

        const AnimationStorage& getStateStorage(RobotState state)
        {
            static std::array<AnimationStorage, 3> states{};
            static const bool initialized = []() {
                createStateAnimation(states[0], "IDLE", neutral);
                createStateAnimation(states[1], "TIRED", tired);
                createStateAnimation(states[2], "SLEEP", eyesClosed);
                return true;
            }();
            (void)initialized;
            return states[static_cast<std::size_t>(state)];
        }
#endif

    }

    const std::array<AnimationTestDefinition, static_cast<std::size_t>(TestAnimationId::Count)>& getAnimationTestDefinitions()
    {
        return animationTests;
    }

    const Animation& getTestAnimation(TestAnimationId id)
    {
        if (isCharacterAnimation(id)) return getCharacterAnimation(id);
#ifdef ROBOT_COMPACT_ANIMATIONS
        initializeAnimations(activeStorage, id);
        return activeStorage.animation;
#else
        return getAnimations()[static_cast<std::size_t>(id)].animation;
#endif
    }

    const Animation& getShakeRecoveryAnimation(const Frame& releaseFrame)
    {
#ifdef ROBOT_COMPACT_ANIMATIONS
        for (std::size_t i = 0; i < activeStorage.animation.frameCount; ++i) {
            if (&releaseFrame == &activeStorage.frames[i]) {
                const Pose start = activeStorage.poses[i];
                DizzyAnimation::build(activeStorage, start);
                return activeStorage.animation;
            }
        }
#else
        const auto& shake = getAnimations()[static_cast<std::size_t>(TestAnimationId::Shaking)];
        for (std::size_t i = 0; i < shake.animation.frameCount; ++i) {
            if (&releaseFrame == &shake.frames[i]) {
                static AnimationStorage recovery{};
                DizzyAnimation::build(recovery, shake.poses[i]);
                return recovery.animation;
            }
        }
#endif
        return getTestAnimation(TestAnimationId::Dizzy);
    }

    const Animation& getTouchReturnAnimation(const Frame& releaseFrame)
    {
        if (const auto* custom = tryTouchReturn(releaseFrame)) return *custom;
        Pose start = neutral;
#ifdef ROBOT_COMPACT_ANIMATIONS
        for (std::size_t i = 0; i < activeStorage.animation.frameCount; ++i)
            if (&releaseFrame == &activeStorage.frames[i]) { start = activeStorage.poses[i]; break; }
        auto& recovery = activeStorage;
#else
        for (const auto& source : getAnimations())
            for (std::size_t i = 0; i < source.animation.frameCount; ++i)
                if (&releaseFrame == &source.frames[i]) start = source.poses[i];
        static AnimationStorage recovery;
#endif
        createAnimation(recovery, "TOUCH RETURN TIRED", {start, tired}, {360, 1}, 2);
        return recovery.animation;
    }

    const AnimationTestDefinition* getStateTransition(RobotState from, RobotState to)
    {
        for (const auto& definition : animationTests)
        {
            if (definition.transition && definition.state == from && definition.targetState == to)
                return &definition;
        }
        return nullptr;
    }

    const Animation& getStateAnimation(RobotState state)
    {
#ifdef ROBOT_COMPACT_ANIMATIONS
        const Pose& pose = state == RobotState::Tired ? tired : state == RobotState::Sleep ? eyesClosed : neutral;
        createStateAnimation(activeStorage, getStateName(state), pose);
        return activeStorage.animation;
#else
        return getStateStorage(state).animation;
#endif
    }

    const char* getStateName(RobotState state)
    {
        switch (state)
        {
        case RobotState::Idle:
            return "IDLE";
        case RobotState::Tired:
            return "TIRED";
        case RobotState::Sleep:
            return "SLEEP";
        }
        return "IDLE";
    }
}

#include "AnimationCatalog.h"
#include "ErrorFaceAnimation.h"
const desktoprobot::Animation& getAnimation(AnimationId id) { return desktoprobot::getTestAnimation(id); }
const desktoprobot::Animation& getErrorAnimation() { return desktoprobot::ErrorFaceAnimation::get(); }

#include "TouchWakeAnimation.h"
#include "TouchLookUpAnimation.h"
const desktoprobot::Animation& getTouchWakeAnimation(desktoprobot::RobotState source,bool lookUp) { return lookUp ? desktoprobot::TouchLookUpAnimation::get(source) : desktoprobot::TouchWakeAnimation::get(source); }
