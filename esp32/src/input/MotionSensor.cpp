#include "MotionSensor.h"
#include <Wire.h>
#include <cmath>
#include <cstdio>
#ifdef ARDUINO
#include <Arduino.h>
#endif

bool MotionSensor::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(Address);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool MotionSensor::begin() {
    available_ = false;
    Wire.beginTransmission(Address);
    Wire.write(0x75); // WHO_AM_I
    if (Wire.endTransmission(false) != 0 || Wire.requestFrom(Address, uint8_t(1)) != 1 || Wire.read() != 0x68)
        return false;

    // Wake the device, select the ±4 g range and a modest hardware low-pass filter.
    if (!writeRegister(0x6B, 0x00) || !writeRegister(0x1C, 0x08) || !writeRegister(0x1A, 0x03) || !writeRegister(0x19, 0x04))
        return false;
    baselineReady_ = false;
    shaking_ = false;
    activityIndex_ = activityCount_ = 0;
    activitySum_ = 0;
    reversals_ = eventRead_ = eventWrite_ = 0;
    previousMotionReady_ = false;
    readFailureCount_ = 0;
    lastSample_ = lastDiagnostic_ = 0;
    available_ = true;
    return true;
}

bool MotionSensor::readAcceleration(float& xG, float& yG, float& zG) {
    Wire.beginTransmission(Address);
    Wire.write(0x3B);
    if (Wire.endTransmission(false) != 0 || Wire.requestFrom(Address, uint8_t(6)) != 6) return false;
    const auto readAxis = []() -> int16_t {
        const uint16_t high = static_cast<uint8_t>(Wire.read());
        const uint16_t low = static_cast<uint8_t>(Wire.read());
        return static_cast<int16_t>((high << 8) | low);
    };
    xG = readAxis() / 8192.0f;
    yG = readAxis() / 8192.0f;
    zG = readAxis() / 8192.0f;
    return true;
}

void MotionSensor::emit(MotionEvent event) {
    const uint8_t next = (eventWrite_ + 1) % 4;
    if (next == eventRead_) return;
    events_[eventWrite_] = event;
    eventWrite_ = next;
}

bool MotionSensor::pollEvent(MotionEvent& event) {
    if (eventRead_ == eventWrite_) return false;
    event = events_[eventRead_];
    eventRead_ = (eventRead_ + 1) % 4;
    return true;
}

void MotionSensor::setSensitivity(MotionSensitivity sensitivity) {
    switch (sensitivity) {
    case MotionSensitivity::LessSensitive: triggerG_ = 0.090f; releaseG_ = 0.040f; break;
    case MotionSensitivity::MoreSensitive: triggerG_ = 0.045f; releaseG_ = 0.018f; break;
    case MotionSensitivity::Normal:
    default: triggerG_ = 0.060f; releaseG_ = 0.025f; break;
    }
}

void MotionSensor::update(uint32_t nowMs) {
    if (!available_ || nowMs - lastSample_ < SampleIntervalMs) return;
    lastSample_ = nowMs;
    float xG, yG, zG;
    if (!readAcceleration(xG, yG, zG)) {
        // Ignore short I2C glitches from loose wires/connectors. Disable the
        // sensor only after a sustained failure instead of losing it on one read.
        if (readFailureCount_ < 10) ++readFailureCount_;
        if (readFailureCount_ >= 10) {
            available_ = false;
            if (shaking_) { shaking_ = false; emit(MotionEvent::SHAKE_END); lastShakeEnd_ = nowMs; }
#ifdef ARDUINO
            if (log_) log_(logContext_,"[ERROR] MPU6050 read failed repeatedly; motion sensor disabled until restart");
            else Serial.println("[ERROR] MPU6050 read failed repeatedly; motion sensor disabled until restart");
#endif
        }
        return;
    }
    readFailureCount_ = 0;
    if (!baselineReady_) {
        previousX_ = xG; previousY_ = yG; previousZ_ = zG;
        baselineX_ = xG; baselineY_ = yG; baselineZ_ = zG;
        baselineReady_ = true;
        return;
    }

    // Remove gravity with a fast baseline, then detect repeated reversals of
    // the dynamic acceleration vector. A single lift or bump cannot satisfy
    // the reversal count, and the baseline settles quickly after movement.
    const float sampleDx = xG - previousX_, sampleDy = yG - previousY_, sampleDz = zG - previousZ_;
    const float sampleChangeG = std::sqrt(sampleDx*sampleDx + sampleDy*sampleDy + sampleDz*sampleDz);
    const float dx = xG - baselineX_, dy = yG - baselineY_, dz = zG - baselineZ_;
    const float dynamicG = std::sqrt(dx*dx + dy*dy + dz*dz);
    baselineX_ += dx * 0.10f;
    baselineY_ += dy * 0.10f;
    baselineZ_ += dz * 0.10f;
    previousX_ = xG; previousY_ = yG; previousZ_ = zG;

    activitySum_ -= activitySamples_[activityIndex_];
    activitySamples_[activityIndex_] = dynamicG;
    activitySum_ += dynamicG;
    activityIndex_ = (activityIndex_ + 1) % AverageSamples;
    if (activityCount_ < AverageSamples) ++activityCount_;
#ifdef ARDUINO
    const float averageG = activitySum_ / activityCount_;
    if (nowMs - lastDiagnostic_ >= 1000) {
        lastDiagnostic_ = nowMs;
        char message[112];
        std::snprintf(message,sizeof(message),"[MOTION] activity=%u mg reversalThreshold=%u mg reversals=%u shaking=%s",
            static_cast<unsigned>(averageG * 1000.0f), static_cast<unsigned>(triggerG_ * 1000.0f),
            reversals_, shaking_ ? "yes" : "no");
        if (log_) log_(logContext_,message); else Serial.println(message);
    }
#endif

    const float previousMotionMagnitude = std::sqrt(previousMotionX_*previousMotionX_
        + previousMotionY_*previousMotionY_ + previousMotionZ_*previousMotionZ_);
    const float directionDot = dx*previousMotionX_ + dy*previousMotionY_ + dz*previousMotionZ_;
    if (previousMotionReady_ && dynamicG >= triggerG_ && previousMotionMagnitude >= triggerG_ && directionDot < 0) {
        if (!reversals_ || nowMs - windowStart_ > WindowMs) {
            windowStart_ = nowMs;
            reversals_ = 0;
        }
        if (reversals_ < 255) ++reversals_;
        if (!shaking_ && reversals_ >= 3 && nowMs - lastShakeEnd_ >= CooldownMs) {
            shaking_ = true;
            lastActivity_ = nowMs;
            emit(MotionEvent::SHAKE_START);
        }
    }
    previousMotionX_ = dx; previousMotionY_ = dy; previousMotionZ_ = dz;
    previousMotionReady_ = true;

    // Do not let the gravity filter's residual keep the shake animation alive.
    // Refresh only while consecutive sensor samples show actual movement.
    if (shaking_ && sampleChangeG > KeepAliveMotionG) lastActivity_ = nowMs;

    if (reversals_ && nowMs - windowStart_ > WindowMs) reversals_ = 0;
    if (shaking_ && nowMs - lastActivity_ >= QuietMs) {
        shaking_ = false;
        lastShakeEnd_ = nowMs;
        reversals_ = 0;
        emit(MotionEvent::SHAKE_END);
    }
}
