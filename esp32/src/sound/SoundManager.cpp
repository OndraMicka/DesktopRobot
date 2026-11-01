#include "SoundManager.h"
#include "../config/Pins.h"
#ifdef ARDUINO
#include <Arduino.h>
#include <esp_timer.h>
#endif

namespace desktoprobot {
namespace {
// ESP32-C3 emulates these small atomics with ESP-IDF critical sections.
// Only scalar mailbox/status operations happen there; never PWM or rendering.
static_assert(Pins::BUZZER != Pins::TOUCH && Pins::BUZZER != Pins::SDA && Pins::BUZZER != Pins::SCL,
              "Buzzer GPIO must not overlap input/display pins");
class PlatformToneOutput final : public ToneOutput {
public:
    bool begin() override {
#ifdef ARDUINO
        pinMode(Pins::BUZZER, OUTPUT);
        digitalWrite(Pins::BUZZER, LOW);
        if (!ledcAttach(Pins::BUZZER, 1000, 10)) return false;
        if (ledcWrite(Pins::BUZZER, 0)) return true;
        ledcDetach(Pins::BUZZER);
        return false;
#else
        return true;
#endif
    }
    bool setFrequency(uint16_t hz) override {
#ifdef ARDUINO
        return hz ? ledcWriteTone(Pins::BUZZER, hz) != 0 : ledcWrite(Pins::BUZZER, 0);
#else
        (void)hz;
        return true;
#endif
    }
};
ToneOutput& platformOutput() { static PlatformToneOutput output; return output; }
}
SoundManager::SoundManager() : output_(platformOutput()) {
#ifdef ARDUINO
    automatic_ = true;
#endif
}
SoundManager::SoundManager(ToneOutput& output) : output_(output) {}
bool SoundManager::begin() {
    if (ready_) return healthy();
    if (!output_.begin() || !output_.setFrequency(0)) return false;
#ifdef ARDUINO
    if (automatic_) {
        esp_timer_create_args_t args{};
        args.callback = timerTick;
        args.arg = this;
        args.dispatch_method = ESP_TIMER_TASK;
        args.name = "robot_sound";
        args.skip_unhandled_events = true;
        esp_timer_handle_t timer = nullptr;
        if (esp_timer_create(&args, &timer) != ESP_OK) return false;
        // One existing ESP Timer task, no dedicated audio task/stack. The
        // short callback owns all PWM changes, independent of OLED transfers.
        if (esp_timer_start_periodic(timer, 2000) != ESP_OK) {
            esp_timer_delete(timer);
            return false;
        }
    }
#endif
    ready_ = true;
    return true;
}
void SoundManager::publish(uint32_t command) {
    auto previous = requested_.load(std::memory_order_relaxed);
    while (!requested_.compare_exchange_weak(previous,
        ((previous + 0x200U) & ~0x1FFU) | command,
        std::memory_order_release, std::memory_order_relaxed)) {}
}
void SoundManager::play(SoundEffect effect, bool loop) {
    if (!healthy() || !getSoundData(effect)) return;
    publish(static_cast<uint32_t>(effect) + 1U + (loop ? 0x100U : 0U));
}
void SoundManager::stop() {
    if (ready_) publish(0);
}
bool SoundManager::isPlaying() const {
    if (!healthy()) return false;
    const auto request = requested_.load(std::memory_order_acquire);
    if (request != processed_.load(std::memory_order_acquire)) return (request & 0xFFU) != 0;
    return playing_.load(std::memory_order_acquire);
}
bool SoundManager::frequency(uint16_t hz) {
    if (output_.setFrequency(hz)) return true;
    output_.setFrequency(0); // Best effort silence; latch the fault, no retry storm.
    failed_.store(true, std::memory_order_release);
    playing_.store(false, std::memory_order_release);
    sound_ = nullptr;
    return false;
}
void SoundManager::advance(uint32_t now) {
    if (failed_.load(std::memory_order_acquire)) return;
    const auto request = requested_.load(std::memory_order_acquire);
    if (request != processed_.load(std::memory_order_relaxed)) {
        const auto id = request & 0xFFU;
        sound_ = id ? getSoundData(static_cast<SoundEffect>(id - 1U)) : nullptr;
        index_ = 0;
        loop_ = (request & 0x100U) != 0;
        segmentStarted_ = now;
        playing_.store(sound_ != nullptr, std::memory_order_release);
        frequency(sound_ ? sound_->segments[0].frequencyHz : 0);
        processed_.store(request, std::memory_order_release);
        return;
    }
    if (!sound_) return;
    uint32_t elapsed = now - segmentStarted_; // millis rollover is intentional.
    if (loop_ && elapsed >= sound_->durationMs) {
        const auto skipped = elapsed / sound_->durationMs * sound_->durationMs;
        segmentStarted_ += skipped;
        elapsed -= skipped;
    }
    const auto previous = index_;
    while (elapsed >= sound_->segments[index_].durationMs) {
        const auto duration = sound_->segments[index_].durationMs;
        elapsed -= duration;
        segmentStarted_ += duration;
        if (++index_ == sound_->count) {
            if (loop_) index_ = 0;
            else {
                sound_ = nullptr;
                playing_.store(false, std::memory_order_release);
                frequency(0);
                return;
            }
        }
    }
    if (index_ != previous) frequency(sound_->segments[index_].frequencyHz);
}
void SoundManager::update(uint32_t now) {
    if (ready_ && !automatic_) advance(now);
}
void SoundManager::timerTick(void* context) {
#ifdef ARDUINO
    static_cast<SoundManager*>(context)->advance(static_cast<uint32_t>(esp_timer_get_time()/1000));
#else
    (void)context;
#endif
}
}
