#include "HumidityPID.h"

HumidityPID::HumidityPID()
    : kp_(2.0f), ki_(0.5f), kd_(0.1f)
    , target_(90.0f)
    , lastError_(0.0f)
    , integral_(0.0f)
    , lastOutput_(0.0f)
    , outMin_(0.0f), outMax_(100.0f)
    , deadband_(1.0f)
    , lastTime_(0)
    , initialized_(false) {}

void HumidityPID::setTarget(float targetRH) {
    target_ = constrain(targetRH, 0.0f, 100.0f);
}

void HumidityPID::setTunings(float kp, float ki, float kd) {
    kp_ = kp;
    ki_ = ki;
    kd_ = kd;
}

void HumidityPID::setOutputLimits(float min, float max) {
    outMin_ = min;
    outMax_ = max;
}

void HumidityPID::setDeadband(float db) {
    deadband_ = db;
}

void HumidityPID::reset() {
    lastError_ = 0.0f;
    integral_ = 0.0f;
    lastOutput_ = 0.0f;
    initialized_ = false;
}

float HumidityPID::compute(float currentRH) {
    if (!lastTime_) {
        lastTime_ = millis();
        lastError_ = target_ - currentRH;
        initialized_ = true;
        return 0.0f;
    }

    unsigned long now = millis();
    float dt = (now - lastTime_) / 1000.0f;
    if (dt <= 0) return lastOutput_;
    lastTime_ = now;

    float error = target_ - currentRH;

    // Deadband: if within deadband, hold current output
    if (fabs(error) < deadband_) {
        return lastOutput_;
    }

    // Proportional
    float pTerm = kp_ * error;

    // Integral with anti-windup
    integral_ += ki_ * error * dt;
    integral_ = constrain(integral_, outMin_, outMax_);

    // Derivative (on measurement, not error - avoids derivative kick)
    float dTerm = kd_ * (lastError_ - error) / dt;

    lastError_ = error;
    lastOutput_ = constrain(pTerm + integral_ + dTerm, outMin_, outMax_);

    return lastOutput_;
}
