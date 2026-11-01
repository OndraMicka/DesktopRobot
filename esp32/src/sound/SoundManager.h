#pragma once
#include "SoundOutput.h"
#include <atomic>

namespace desktoprobot {
// Hardware boundary also permits deterministic host tests without Arduino.
class ToneOutput {
public:
    virtual ~ToneOutput() = default;
    virtual bool begin() = 0;
    virtual bool setFrequency(uint16_t frequencyHz) = 0;
};

class SoundManager final : public SoundOutput {
public:
    // The default ESP32 instance owns a device-lifetime timer. Keep its owner
    // alive for the life of the firmware (Robot is global in main.cpp).
    SoundManager();
    // Injected outputs use manual update(), including on ESP32.
    explicit SoundManager(ToneOutput& output);
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    bool begin();
    void play(SoundEffect effect, bool loop = false) override;
    void stop() override;
    bool isPlaying() const override;
    bool healthy() const { return ready_ && !failed_.load(std::memory_order_acquire); }
    void update(uint32_t now); // Manual instances only; never call from an ISR.
private:
    ToneOutput& output_;
    bool automatic_ = false, ready_ = false;
    // One latest-request mailbox: replacement cancels obsolete commands.
    // Bits 0..7: effect+1 (0=stop), bit 8: loop, bits 9..31: generation.
    std::atomic<uint32_t> requested_{0}, processed_{0};
    std::atomic<bool> playing_{false}, failed_{false};
    // Only the timer task (or manual update) touches the sequencer and LEDC.
    const SoundData* sound_ = nullptr;
    std::size_t index_ = 0;
    uint32_t segmentStarted_ = 0;
    bool loop_ = false;
    void publish(uint32_t command);
    void advance(uint32_t now);
    bool frequency(uint16_t hz);
    static void timerTick(void* context);
};
}
