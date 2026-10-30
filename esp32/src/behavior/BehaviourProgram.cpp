#include "BehaviourProgram.h"

BehaviourProgram::BehaviourProgram(AnimationOutput& output, BehaviourConfig config, Random random, void* context)
    : output_(output), config_(config), random_(random), randomContext_(context) { rng_ = config.randomSeed ? config.randomSeed : 1; }

uint32_t BehaviourProgram::randomValue() {
    if (random_) return random_(randomContext_);
    rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
    return rng_;
}
uint32_t BehaviourProgram::range(uint32_t min, uint32_t max) {
    return min + (max > min ? randomValue() % (max - min + 1) : 0);
}
void BehaviourProgram::resetTimers() {
    const auto index = static_cast<unsigned>(state_);
    if (index >= 3) return;
    activityStart_ = blinkStart_ = now_;
    activityDelay_ = range(config_.activityMinMs[index], config_.activityMaxMs[index]);
    if (index < 2) blinkDelay_ = range(config_.blinkMinMs[index], config_.blinkMaxMs[index]);
}
void BehaviourProgram::base() {
    mode_ = Mode::Base;
    sequenceCount_ = 0; quickWake_=false;
    output_.playBase(state_, now_);
    resetTimers();
}
void BehaviourProgram::begin(uint32_t now, LocalTime local) {
    now_ = now; local_ = local;
    base();
    checkSchedule();
}
void BehaviourProgram::start(Mode mode, const AnimationId* ids, unsigned count, bool holdLast) {
    quickWake_=false;
    mode_ = mode;
    sequenceCount_ = count;
    sequenceIndex_ = 0;
    holdLast_ = holdLast;
    for (unsigned i = 0; i < count; ++i) sequence_[i] = ids[i];
    playStep();
}
void BehaviourProgram::transition(RobotState target, Mode mode, bool morning) {
    const auto source = state_;
    state_ = target; // State indicators show destination throughout the transition.
    const auto* definition = desktoprobot::getStateTransition(
        static_cast<desktoprobot::RobotState>(source), static_cast<desktoprobot::RobotState>(target));
    AnimationId ids[4];
    unsigned count = 0;
    if (definition) ids[count++] = definition->id;
    if (morning) {
        ids[count++] = AnimationId::IdleYawn;
        ids[count++] = AnimationId::LookAround;
    }
    if (count) start(mode, ids, count);
    else base();
}
void BehaviourProgram::setState(RobotState target) {
    if (state_ == RobotState::ERROR) return;
    if (target == RobotState::ERROR) {
        state_ = target; mode_ = Mode::Base; held_ = false;
        sequenceCount_ = 0; output_.showErrorFace(); return;
    }
    if (target == state_) return;
    held_ = false; temporary_ = Temporary::None;
    transition(target, Mode::Transition);
}
void BehaviourProgram::preview(AnimationId id) {
    if (state_ == RobotState::ERROR) return;
    for (const auto& definition : desktoprobot::getAnimationTestDefinitions()) {
        if (definition.id == id && definition.transition) {
            state_ = static_cast<RobotState>(definition.targetState);
            temporary_ = Temporary::None; held_ = false;
            start(Mode::Transition, &id, 1);
            return;
        }
    }
    start(Mode::Activity, &id, 1);
}
void BehaviourProgram::checkSchedule() {
    if (!local_.valid || local_.second >= 86400) return;
    const auto sec = local_.second;
    const int phase = sec < 7*3600 ? -1 : sec < 21*3600 ? 0 : sec < 21*3600+1800 ? 1 : 2;
    // Midnight is not a schedule boundary; the last boundary is previous bedtime.
    const int64_t boundary = phase < 0 ? (local_.day-1)*3+2 : local_.day*3+phase;
    if (scheduleKnown_ && boundary == scheduleBoundary_) return;
    const bool morning = phase == 0 && (scheduleKnown_ || state_ != RobotState::IDLE);
    scheduleKnown_ = true; scheduleBoundary_ = boundary;
    temporary_ = Temporary::None; held_ = false;
    const auto target = phase == 0 ? RobotState::IDLE : phase == 1 ? RobotState::TIRED : RobotState::SLEEP;
    if (target != state_ || mode_ != Mode::Base || morning)
        transition(target, Mode::Schedule, morning);
}
void BehaviourProgram::handleEvent(InputEvent event) {
    if (state_ == RobotState::ERROR || event == InputEvent::NONE) return;
    if (event == InputEvent::RELEASE) {
        held_ = false;
        if (mode_ == Mode::Reaction && holdLast_) finishReaction();
        return;
    }
    if (mode_ == Mode::Schedule) return;
    if (event == InputEvent::SHAKE_START) {
        if (mode_ == Mode::Shake) return;
        held_ = false;
        const auto id = AnimationId::Shaking;
        start(Mode::Shake, &id, 1); return;
    }
    if (event == InputEvent::SHAKE_END) {
        if (mode_ == Mode::Shake) {
            mode_ = Mode::Dizzy; sequenceCount_ = 0;
            output_.playShakeRecovery(now_);
        }
        return;
    }
    if (mode_ == Mode::Shake || mode_ == Mode::Dizzy) return;
    if (event != InputEvent::TAP && event != InputEvent::HOLD) return;
    const bool wasOpening=quickWake_;
    const auto origin=state_;
    returnState_ = origin;
    held_ = event == InputEvent::HOLD;
    AnimationId ids[4]; unsigned count = 0;

    if (held_) { ids[count++] = AnimationId::LookUp; ids[count++] = AnimationId::Smile; }
    else ids[count++] = AnimationId::Angry;
    if (origin == RobotState::SLEEP) {
        returnState_ = RobotState::TIRED;
        temporary_ = local_.valid && local_.second < 7*3600 ? Temporary::Early : Temporary::TiredThenSleep;
    } else if (origin == RobotState::TIRED && !held_) {
        if (temporary_ != Temporary::Early) {
            returnState_ = RobotState::IDLE;
            temporary_ = Temporary::IdleThenTired;
        }
    }
    output_.interactionSound(held_);
    if (origin!=RobotState::IDLE) {
        mode_=Mode::Reaction; sequenceCount_=count; sequenceIndex_=0; holdLast_=held_;
        for(unsigned i=0;i<count;++i) sequence_[i]=ids[i];
        quickWake_=true;
        if(!wasOpening) output_.playTouchWake(origin,now_);
    } else start(Mode::Reaction, ids, count, held_);
}
void BehaviourProgram::finishReaction() {
    if (temporary_ == Temporary::Early) returnState_ = RobotState::TIRED;
    if (state_ != returnState_) temporaryStart_ = now_;
    state_ = returnState_;
    base();
}
void BehaviourProgram::chooseActivity() {
    uint32_t total = config_.quietWeight;
    const auto eligible = [&](unsigned i, RobotState state, uint32_t cooldown) {
        return state == state_ && static_cast<int>(i) != lastActivity_
            && (!played_[i] || now_-lastPlayed_[i] >= cooldown);
    };
    for (unsigned i = 0; i < config_.activities.size(); ++i) {
        const auto& a = config_.activities[i];
        if (eligible(i, a.state, a.cooldownMs)) total += a.weight;
    }
    if (eligible(13, RobotState::SLEEP, config_.sleepPeekCooldownMs)) total += config_.sleepPeekWeight;
    uint32_t choice = total ? randomValue()%total : 0;
    if (choice < config_.quietWeight || !total) { resetTimers(); return; }
    choice -= config_.quietWeight;
    for (unsigned i = 0; i < 14; ++i) {
        const auto a = i == 13 ? Activity{AnimationId::SleepEyeOpen, RobotState::SLEEP, config_.sleepPeekWeight, config_.sleepPeekCooldownMs} : config_.activities[i];
        if (!eligible(i, a.state, a.cooldownMs)) continue;
        if (choice < a.weight) {
            played_[i] = true; lastPlayed_[i] = now_; lastActivity_ = static_cast<int>(i);
            start(Mode::Activity, &a.id, 1); return;
        }
        choice -= a.weight;
    }
    resetTimers();
}
void BehaviourProgram::update(uint32_t now, LocalTime local) {
    if (state_ == RobotState::ERROR) return;
    now_ = now; local_ = local;
    checkSchedule();
    if (mode_ == Mode::Shake) return;
    if (mode_ != Mode::Base) {
        if (!output_.isAnimationFinished()) return;
        if (quickWake_) { quickWake_=false; playStep(); return; }
        if (sequenceCount_ && sequenceIndex_+1 < sequenceCount_) {
            ++sequenceIndex_;
            playStep();
            return;
        }
        if (mode_ == Mode::Reaction) {
            if (!held_) finishReaction();
            return;
        }
        const bool wasBlink = mode_ == Mode::Blink;
        const auto activityStart = activityStart_, activityDelay = activityDelay_;
        base();
        if (wasBlink) { activityStart_ = activityStart; activityDelay_ = activityDelay; }
    }
    if (temporary_ != Temporary::None && temporary_ != Temporary::Early && now_-temporaryStart_ >= config_.temporaryMs) {
        const bool toTired = temporary_ == Temporary::IdleThenTired;
        temporary_ = toTired ? Temporary::TiredThenSleep : Temporary::None;
        temporaryStart_ = now_;
        transition(toTired ? RobotState::TIRED : RobotState::SLEEP, Mode::Transition);
        return;
    }
    if (!spontaneous_) return;
    if (now_-activityStart_ >= activityDelay_) { chooseActivity(); return; }
    if (state_ != RobotState::SLEEP && now_-blinkStart_ >= blinkDelay_) {
        const auto id = state_ == RobotState::IDLE ? AnimationId::Blink : AnimationId::SlowBlink;
        start(Mode::Blink, &id, 1);
    }
}

void BehaviourProgram::requestState(RobotState target, bool morning) {
    if (state_ == RobotState::ERROR || mode_ == Mode::Schedule) return;
    if (target == state_ && mode_ == Mode::Base && !morning) return;
    held_ = false; temporary_ = Temporary::None;
    transition(target, Mode::Transition, morning && target == RobotState::IDLE);
}
void BehaviourProgram::changeTime(uint32_t now, LocalTime local, bool restoreSchedule) {
    if (state_ == RobotState::ERROR) return;
    now_ = now; local_ = local;
    if (restoreSchedule) {
        temporary_ = Temporary::None; held_ = false;
        if (!local.valid) {
            scheduleKnown_ = false;
            if (state_ != RobotState::IDLE) transition(RobotState::IDLE, Mode::Transition);
            else base();
            return;
        }
        const auto target = local.second < 25200 || local.second >= 77400 ? RobotState::SLEEP
            : local.second >= 75600 ? RobotState::TIRED : RobotState::IDLE;
        if (!(mode_ == Mode::Schedule && target == state_)) scheduleKnown_ = false;
    }
    checkSchedule();
}
const char* BehaviourProgram::modeName() const {
    const char* names[] = {"BASE","ACTIVITY","BLINK","REACTION","TRANSITION","SCHEDULE","SHAKE","DIZZY"};
    return names[static_cast<unsigned>(mode_)];
}
const char* BehaviourProgram::temporaryName() const {
    const char* names[] = {"NONE","IDLE_THEN_TIRED","TIRED_THEN_SLEEP","EARLY_UNTIL_07"};
    return names[static_cast<unsigned>(temporary_)];
}
uint32_t BehaviourProgram::temporaryRemainingMs() const {
    if (temporary_ == Temporary::None || temporary_ == Temporary::Early) return 0;
    const auto elapsed = now_-temporaryStart_;
    return elapsed < config_.temporaryMs ? config_.temporaryMs-elapsed : 0;
}

void BehaviourProgram::playStep() {
 if(mode_==Mode::Reaction && sequence_[sequenceIndex_]==AnimationId::LookUp) output_.playTouchLookUp(now_);
 else output_.playAnimation(sequence_[sequenceIndex_],now_,holdLast_ && sequenceIndex_+1==sequenceCount_);
}
