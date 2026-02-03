#pragma once
#include <Arduino.h>
#include "AdvancedPID.h"

struct PIDParams;  // Forward declaration

class TemperatureController {
public:
    TemperatureController(uint8_t outputPin);
    static struct PIDParams getDefaultPIDParams();  // Valori di default centralizzati
    void setSetpoint(float temp);
    void setPIDParameters(float kp, float ki, float kd);
    void setOutputLimits(float min, float max);
    void enable();
    void disable();
    void update(float currentTemp);
    float getOutput() const { return output; }
    bool isOutputActive() const { return outputActive; }
    void setAdvancedPIDParams(const struct PIDParams &params);
    void resetIntegral();
    PID_Handle* getPIDHandle() { return &pid; }
private:
    uint8_t pin;
    bool enabled;
    PID_Handle pid;
    float output;  // 0.0 to 100.0 (percentage)
    float currentSetpoint;
    unsigned long cycleStartTime;
    bool outputActive;
    unsigned long cyclePeriodMs;  // Periodo PWM configurabile
    unsigned long minPulseMs;     // Impulso minimo configurabile
    void updateOutput();
};
