#pragma once

#include <Arduino.h>

class TemperatureController {
public:
    TemperatureController(uint8_t outputPin);
    
    // Configuration
    void setSetpoint(float temp);
    float getSetpoint() const { return setpoint; }
    
    void setPIDParameters(float kp, float ki, float kd);
    void setOutputLimits(float min, float max);
    
    // Control
    void enable();
    void disable();
    bool isEnabled() const { return enabled; }
    
    // Update (call this regularly in loop)
    void update(float currentTemp);
    
    // Status
    float getOutput() const { return output; }
    bool isOutputActive() const { return outputActive; }
    
private:
    uint8_t pin;
    float setpoint;
    bool enabled;
    
    // PID parameters
    float Kp, Ki, Kd;
    float outputMin, outputMax;
    
    // PID state
    float integral;
    float lastError;
    unsigned long lastUpdateTime;
    
    // Time-based PWM with minimum pulse width
    float output;  // 0.0 to 100.0 (percentage)
    unsigned long cycleStartTime;
    bool outputActive;
    
    static const unsigned long CYCLE_PERIOD_MS = 30000;  // 30 seconds
    static const unsigned long MIN_PULSE_MS = 1000;      // 1 second minimum
    
    void updateOutput();
    void resetIntegral();
};
