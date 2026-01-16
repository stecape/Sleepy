#include "TemperatureController.h"

TemperatureController::TemperatureController(uint8_t outputPin) 
    : pin(outputPin), setpoint(25.0), enabled(false),
      Kp(10.0), Ki(0.5), Kd(5.0),  // Default PID values (will need tuning)
      outputMin(0.0), outputMax(100.0),
      integral(0.0), lastError(0.0), lastUpdateTime(0),
      output(0.0), cycleStartTime(0), outputActive(false)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void TemperatureController::setSetpoint(float temp) {
    if (temp < 10.0) temp = 10.0;
    if (temp > 50.0) temp = 50.0;
    setpoint = temp;
}

void TemperatureController::setPIDParameters(float kp, float ki, float kd) {
    Kp = kp;
    Ki = ki;
    Kd = kd;
}

void TemperatureController::setOutputLimits(float min, float max) {
    outputMin = min;
    outputMax = max;
}

void TemperatureController::enable() {
    if (!enabled) {
        enabled = true;
        resetIntegral();
        cycleStartTime = millis();
        lastUpdateTime = millis();
    }
}

void TemperatureController::disable() {
    enabled = false;
    output = 0.0;
    digitalWrite(pin, LOW);
    outputActive = false;
}

void TemperatureController::resetIntegral() {
    integral = 0.0;
    lastError = 0.0;
}

void TemperatureController::update(float currentTemp) {
    if (!enabled) {
        output = 0.0;
        digitalWrite(pin, LOW);
        outputActive = false;
        return;
    }
    
    unsigned long now = millis();
    
    // Calculate PID every second or on first run
    if (lastUpdateTime == 0 || (now - lastUpdateTime) >= 1000) {
        float dt = (now - lastUpdateTime) / 1000.0; // Convert to seconds
        if (lastUpdateTime == 0) dt = 1.0; // First run
        
        // Calculate error
        float error = setpoint - currentTemp;
        
        // Proportional term
        float P = Kp * error;
        
        // Integral term with anti-windup
        // Only integrate if output is not saturated or error is reducing saturation
        if ((output >= outputMin && output <= outputMax) || 
            (output <= outputMin && error > 0) || 
            (output >= outputMax && error < 0)) {
            integral += error * dt;
            
            // Additional anti-windup: clamp integral
            float maxIntegral = 100.0 / Ki;  // Prevent integral from dominating
            if (integral > maxIntegral) integral = maxIntegral;
            if (integral < -maxIntegral) integral = -maxIntegral;
        }
        float I = Ki * integral;
        
        // Derivative term
        float derivative = (error - lastError) / dt;
        float D = Kd * derivative;
        
        // Calculate output
        output = P + I + D;
        
        // Clamp output to limits
        if (output < outputMin) output = outputMin;
        if (output > outputMax) output = outputMax;
        
        // Store for next iteration
        lastError = error;
        lastUpdateTime = now;
    }
    
    // Update time-based PWM output
    updateOutput();
}

void TemperatureController::updateOutput() {
    unsigned long now = millis();
    unsigned long cycleElapsed = now - cycleStartTime;
    
    // Check if we need to start a new cycle
    if (cycleElapsed >= CYCLE_PERIOD_MS) {
        cycleStartTime = now;
        cycleElapsed = 0;
    }
    
    // Calculate ON time for this cycle based on output percentage
    unsigned long onTime = (unsigned long)((output / 100.0) * CYCLE_PERIOD_MS);
    
    // Apply minimum pulse width constraint
    if (onTime > 0 && onTime < MIN_PULSE_MS) {
        onTime = 0;  // Too short, don't turn on
    }
    
    // Determine if output should be ON or OFF
    if (cycleElapsed < onTime) {
        if (!outputActive) {
            digitalWrite(pin, HIGH);
            outputActive = true;
        }
    } else {
        if (outputActive) {
            digitalWrite(pin, LOW);
            outputActive = false;
        }
    }
}
