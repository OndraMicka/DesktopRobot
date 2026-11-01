#include "BehaviourProgram.h"

BehaviourProgram::BehaviourProgram(AnimationOutput& output, BehaviourConfig config, Random random, void* context)
    : output_(output), config_(config), random_(random), randomContext_(context) { rng_ = config.randomSeed ? config.randomSeed : 1; }

BehaviourProgram::BehaviourProgram(AnimationOutput& output, desktoprobot::SoundOutput& sound,
    BehaviourConfig config, Random random, void* context)
    : BehaviourProgram(output, config, random, context) { sound_ = &sound; }

void BehaviourProgram::stopSound() { if (sound_) sound_->stop(); }
void BehaviourProgram::playSound(desktoprobot::SoundEffect effect, bool loop) {
    if (sound_ && soundsEnabled_) sound_->play(effect, loop);
}

uint32_t BehaviourProgram::randomValue() {
    if (random_) return random_(randomContext_);
    rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
    return rng_;
}
uint32_t BehaviourProgram::range(uint32_t min, uint32_t max) {
    return min + (max > min ? randomValue() % (max - min + 1) : 0);
}
void BehaviourProgram::resetTimers(bool resetBlink) {
    const auto index = static_cast<unsigned>(state_);
    if (index >= 3) return;
    activityStart_ = now_;
    activityDelay_ = range(config_.activityMinMs[index], config_.activityMaxMs[index]);
    if (resetBlink && index < 2) {
        blinkStart_ = now_;
        blinkDelay_ = range(config_.blinkMinMs[index], config_.blinkMaxMs[index]);
    }
}
void BehaviourProgram::base(bool resetBlink) {
    stopSound();
    mode_ = Mode::Base;
    sequenceCount_ = 0; quickWake_=false;
    spontaneousDelay_ = 0;
    output_.playBase(state_, now_);
    resetTimers(resetBlink);
}
void BehaviourProgram::begin(uint32_t now, LocalTime local) {
    now_ = now; local_ = local;
    soundsEnabled_ = false; // Initial schedule reconciliation is silent.
    base();
    checkSchedule();
    soundsEnabled_ = true;
}
void BehaviourProgram::start(Mode mode, const AnimationId* ids, unsigned count, bool holdLast) {
    stopSound();
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
    if (definition) {
        if (target == RobotState::IDLE) playSound(desktoprobot::SoundEffect::WakeUp);
        else if (target == RobotState::SLEEP) playSound(desktoprobot::SoundEffect::FallAsleep);
    }
}
void BehaviourProgram::setState(RobotState target) {
    if (state_ == RobotState::ERROR) return;
    if (target == RobotState::ERROR) {
        stopSound();
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
        start(Mode::Shake, &id, 1);
        playSound(desktoprobot::SoundEffect::Shake1, true); return;
    }
    if (event == InputEvent::SHAKE_END) {
        if (mode_ == Mode::Shake) {
            stopSound();
            mode_ = Mode::Dizzy; sequenceCount_ = 0;
            output_.playShakeRecovery(now_);
        }
        return;
    }
    if (mode_ == Mode::Shake || mode_ == Mode::Dizzy) return;
    if (event != InputEvent::TAP && event != InputEvent::HOLD) return;
    const bool wasOpening=quickWake_;
    const auto origin=state_;
    // A quick tap opening already ends at neutral: do not close the eyes again
    // if a hold replaces its reaction tail.
    reactionSource_ = wasOpening ? RobotState::IDLE : origin;
    returnState_ = origin;
    held_ = event == InputEvent::HOLD;
    AnimationId ids[4]; unsigned count = 0;

    if (held_) { ids[count++] = AnimationId::LookUp; ids[count++] = AnimationId::Smile; }
    else ids[count++] = AnimationId::Angry;
    if (origin == RobotState::SLEEP) {
        returnState_ = RobotState::TIRED;
        temporary_ = local_.valid && local_.second < 7*3600 ? Temporary::Early : Temporary::TiredThenSleep;
    } else if (origin == RobotState::TIRED) {
        if (temporary_ != Temporary::Early) {
            returnState_ = RobotState::IDLE;
            temporary_ = Temporary::IdleThenTired;
        }
    }
    stopSound();
    if (origin!=RobotState::IDLE && (!held_ || wasOpening)) {
        mode_=Mode::Reaction; sequenceCount_=count; sequenceIndex_=0; holdLast_=held_;
        for(unsigned i=0;i<count;++i) sequence_[i]=ids[i];
        quickWake_=true;
        if(!wasOpening) output_.playTouchWake(origin,now_);
    } else start(Mode::Reaction, ids, count, held_);
    if (!held_) playSound(desktoprobot::SoundEffect::Tap1);
}
void BehaviourProgram::finishReaction() {
    stopSound();
    if (temporary_ == Temporary::Early) returnState_ = RobotState::TIRED;
    if (state_ != returnState_) temporaryStart_ = now_;
    state_ = returnState_;
    if (state_ == RobotState::TIRED) {
        mode_ = Mode::TouchReturn; sequenceCount_ = 0; quickWake_ = false;
        output_.playTouchReturn(now_);
        return;
    }
    base();
}
void BehaviourProgram::chooseActivity() {
    uint32_t total = config_.quietWeight;
    const auto isStartle = [](AnimationId id) {
        return id == AnimationId::StartleLeft || id == AnimationId::StartleUpRight;
    };
    const auto activityAt = [&](unsigned i) {
        return i == config_.activities.size()
            ? Activity{AnimationId::SleepEyeOpen, RobotState::SLEEP, config_.sleepPeekWeight, config_.sleepPeekCooldownMs}
            : config_.activities[i];
    };
    const auto eligible = [&](unsigned i, const Activity& a) {
        return a.state == state_ && static_cast<int>(i) != lastActivity_
            && (!played_[i] || now_-lastPlayed_[i] >= a.cooldownMs)
            && (!isStartle(a.id) || !startlePlayed_ || now_-lastStartle_ >= config_.startleGroupCooldownMs);
    };
    for (unsigned i = 0; i < played_.size(); ++i) {
        const auto a = activityAt(i);
        if (eligible(i, a)) total += a.weight;
    }
    uint32_t choice = total ? randomValue()%total : 0;
    if (choice < config_.quietWeight || !total) { resetTimers(false); return; }
    choice -= config_.quietWeight;
    for (unsigned i = 0; i < played_.size(); ++i) {
        const auto a = activityAt(i);
        if (!eligible(i, a)) continue;
        if (choice < a.weight) {
            played_[i] = true; lastPlayed_[i] = now_; lastActivity_ = static_cast<int>(i);
            if (isStartle(a.id)) { startlePlayed_ = true; lastStartle_ = now_; }
            start(Mode::Activity, &a.id, 1); return;
        }
        choice -= a.weight;
    }
    resetTimers(false);
}
void BehaviourProgram::update(uint32_t now, LocalTime local) {
    if (state_ == RobotState::ERROR) return;
    now_ = now; local_ = local;
    checkSchedule();
    if (mode_ == Mode::Shake) return;
    // A press has priority even before its duration distinguishes TAP/HOLD.
    // Cancel only spontaneous playback; reactions and shake recovery keep theirs.
    if (touchPending_ && (mode_ == Mode::Activity || mode_ == Mode::Blink)) base();
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
        const bool wasActivity = mode_ == Mode::Activity;
        const auto activityStart = activityStart_, activityDelay = activityDelay_;
        base(!wasActivity);
        if (wasBlink) { activityStart_ = activityStart; activityDelay_ = activityDelay; }
        if (wasBlink || wasActivity) {
            spontaneousStart_ = now_;
            spontaneousDelay_ = range(config_.spontaneousGapMinMs, config_.spontaneousGapMaxMs);
        }
    }
    if (temporary_ != Temporary::None && temporary_ != Temporary::Early && now_-temporaryStart_ >= config_.temporaryMs) {
        const bool toTired = temporary_ == Temporary::IdleThenTired;
        temporary_ = toTired ? Temporary::TiredThenSleep : Temporary::None;
        temporaryStart_ = now_;
        transition(toTired ? RobotState::TIRED : RobotState::SLEEP, Mode::Transition);
        return;
    }
    if (!spontaneous_ || touchPending_) return;
    if (now_-spontaneousStart_ < spontaneousDelay_) return;
    // Keep both independent deadlines, but leave a random breath between
    // gestures. If both are due, serve the older deadline; randomize exact ties.
    const uint32_t activityElapsed = now_-activityStart_, blinkElapsed = now_-blinkStart_;
    const bool activityDue = activityElapsed >= activityDelay_;
    const bool blinkDue = state_ != RobotState::SLEEP && blinkElapsed >= blinkDelay_;
    if (activityDue && (!blinkDue || activityElapsed-activityDelay_ > blinkElapsed-blinkDelay_
        || (activityElapsed-activityDelay_ == blinkElapsed-blinkDelay_ && (randomValue() & 1U)))) {
        chooseActivity(); return;
    }
    if (blinkDue) {
        const auto id = state_ == RobotState::IDLE ? AnimationId::Blink : AnimationId::SlowBlink;
        start(Mode::Blink, &id, 1);
        return;
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
    const char* names[] = {"BASE","ACTIVITY","BLINK","REACTION","TRANSITION","SCHEDULE","SHAKE","DIZZY","TOUCH_RETURN"};
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
 if(mode_==Mode::Reaction && sequence_[sequenceIndex_]==AnimationId::LookUp) output_.playTouchLookUp(reactionSource_,now_);
 else output_.playAnimation(sequence_[sequenceIndex_],now_,
     (holdLast_ && sequenceIndex_+1==sequenceCount_) ||
     (mode_==Mode::Reaction && sequence_[sequenceIndex_]==AnimationId::Angry && returnState_==RobotState::TIRED));
 // Sound belongs to the smile step, not the preceding opening/upward gaze.
 // playStep runs only on entry, so a held smile cannot restart the sound.
 if(mode_==Mode::Reaction && held_ && sequence_[sequenceIndex_]==AnimationId::Smile)
     playSound(desktoprobot::SoundEffect::Hold);
}
