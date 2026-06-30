// Simple PID controller for humidity management
// Controls humidifier based on current vs target RH

#ifndef HUMIDITY_PID_H
#define HUMIDITY_PID_H

#include <Arduino.h>

class HumidityPID {
public:
    HumidityPID();
    ~HumidityPID() = default;

    void setTarget(float targetRH);
    void setTunings(float kp, float ki, float kd);
    void setOutputLimits(float min, float max);
    void setDeadband(float db);  // Avoid oscillation near target

    float compute(float currentRH);
    void reset();

    float getOutput() const { return lastOutput_; }
    float getTarget() const { return target_; }

private:
    float kp_, ki_, kd_;
    float target_;
    float lastError_;
    float integral_;
    float lastOutput_;
    float outMin_, outMax_;
    float deadband_;
    unsigned long lastTime_;

    bool initialized_;
};

#endif // HUMIDITY_PID_H
