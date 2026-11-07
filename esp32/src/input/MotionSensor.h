#pragma once

#include <cstdint>

enum class MotionEvent : uint8_t { SHAKE_START, SHAKE_END };
enum class MotionSensitivity : uint8_t { LessSensitive, Normal, MoreSensitive };

// Non-blocking MPU6050 input. The caller owns the I2C bus lifecycle.
class MotionSensor {
public:
    bool begin();
    void update(uint32_t nowMs);
    bool pollEvent(MotionEvent& event);
    bool available() const { return available_; }
    void setSensitivity(MotionSensitivity sensitivity);

private:
    static constexpr uint8_t Address = 0x68;
    static constexpr uint32_t SampleIntervalMs = 10;
    static constexpr uint32_t WindowMs = 1500;
    static constexpr uint32_t QuietMs = 150;
    static constexpr uint32_t CooldownMs = 900;
    static constexpr float KeepAliveMotionG = 0.035f;
    float triggerG_ = 0.060f, releaseG_ = 0.025f;

    static constexpr uint8_t AverageSamples = 5;
    bool available_ = false, baselineReady_ = false;
    bool shaking_ = false, previousMotionReady_ = false;
    float previousX_ = 0, previousY_ = 0, previousZ_ = 1.0f;
    float baselineX_ = 0, baselineY_ = 0, baselineZ_ = 1.0f;
    float previousMotionX_ = 0, previousMotionY_ = 0, previousMotionZ_ = 0;
    float activitySamples_[AverageSamples]{};
    float activitySum_ = 0;
    uint8_t activityIndex_ = 0, activityCount_ = 0;
    uint32_t lastSample_ = 0, windowStart_ = 0, lastActivity_ = 0, lastShakeEnd_ = 0;
    uint8_t reversals_ = 0;
    uint8_t readFailureCount_ = 0;
    uint32_t lastDiagnostic_ = 0;
    MotionEvent events_[4]{};
    uint8_t eventRead_ = 0, eventWrite_ = 0;

    bool writeRegister(uint8_t reg, uint8_t value);
    bool readAcceleration(float& xG, float& yG, float& zG);
    void emit(MotionEvent event);
};
