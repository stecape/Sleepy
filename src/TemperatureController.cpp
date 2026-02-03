#include "TemperatureController.h"
#include "WebServer.h"
#include <string.h>

PIDParams TemperatureController::getDefaultPIDParams() {
    // Valori di default centralizzati per tutti i parametri PID
    return {1.0, 40.0, 0.0, 0.0, 1.0, 0.0, 100.0, 0.0, 100.0, 0.0, 0.0, 20.0, 0.0, 0.0, 30000, 1000, false, false};
}

TemperatureController::TemperatureController(uint8_t outputPin)
    : pin(outputPin), enabled(false), output(0.0), currentSetpoint(20.0), cycleStartTime(0), outputActive(false),
      cyclePeriodMs(30000), minPulseMs(1000)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    
    // Inizializza PID_Handle con valori di default
    memset(&pid, 0, sizeof(PID_Handle));
    pid.params.Kp = 1.0f;
    pid.params.Gp = 40.0f;
    pid.params.Ti = 0.0f;  // Integrativa disabilitata
    pid.params.Td = 0.0f;
    pid.params.Taw = 1.0f;
    pid.params.dt = 0.1f;  // Sample time fisso: 100ms
    pid.params.out_min = 0.0f;
    pid.params.out_max = 100.0f;
    pid.params.ref_min = 0.0f;
    pid.params.ref_max = 100.0f;
    pid.params.gradiente = 0.0f;
    pid.params.output_gradient = 0.0f;
    pid.state.debounce_threshold = PID_DEBOUNCE_DEFAULT;
}

void TemperatureController::setSetpoint(float temp) {
    currentSetpoint = temp;
}

void TemperatureController::setPIDParameters(float kp, float ki, float kd) {
    pid.params.Kp = kp;
    pid.params.Gp = 40.0f;
    pid.params.Ti = 0.0f;  // Integrativa disabilitata
    pid.params.Td = kd;
    pid.params.Taw = 10.0f;
    pid.params.dt = 0.1f;  // Sample time fisso: 100ms
    pid.params.out_min = 0.0f;
    pid.params.out_max = 100.0f;
    pid.params.ref_min = 0.0f;
    pid.params.ref_max = 100.0f;
    pid.params.gradiente = 0.0f;
    pid.params.output_gradient = 0.0f;
}

void TemperatureController::setOutputLimits(float min, float max) {
    pid.params.out_min = min;
    pid.params.out_max = max;
}

void TemperatureController::setAdvancedPIDParams(const PIDParams &params) {
    pid.params.Kp = params.Kp;
    pid.params.Gp = params.Gp;
    pid.params.Ti = params.Ti;
    pid.params.Td = params.Td;
    pid.params.Taw = params.Taw;
    pid.params.dt = 0.1f;  // Sample time fisso: 100ms
    pid.params.out_min = params.out_min;
    pid.params.out_max = params.out_max;
    pid.params.ref_min = params.ref_min;
    pid.params.ref_max = params.ref_max;
    pid.params.gradiente = params.gradiente;
    pid.params.output_gradient = params.output_gradient;
    currentSetpoint = params.setpoint;
    cyclePeriodMs = params.cycle_period_ms;
    minPulseMs = params.min_pulse_ms;
}

void TemperatureController::enable() {
    if (!enabled) {
        enabled = true;
        // Reset stato PID
        pid.state.integrale = 0.0f;
        pid.state.errore_prec = 0.0f;
        pid.state.out = 0.0f;
        pid.state.out_pid = 0.0f;
        pid.state.out_tot = 0.0f;
        pid.state.output_ramp = 0.0f;
        pid.state.ramp_initialized = false;
        pid.state.output_ramp_initialized = false;
        pid.state.debounce_counter = 0;  // Reset debounce
        cycleStartTime = millis();
    }
}

void TemperatureController::disable() {
    enabled = false;
    output = 0.0;
    digitalWrite(pin, LOW);
    outputActive = false;
    
    // Reset completo stato PID
    pid.state.integrale = 0.0f;
    pid.state.errore_prec = 0.0f;
    pid.state.error = 0.0f;
    pid.state.out = 0.0f;
    pid.state.out_pid = 0.0f;
    pid.state.out_tot = 0.0f;
    pid.state.proportionalCorrection = 0.0f;
    pid.state.integralCorrection = 0.0f;
    pid.state.derivativeCorrection = 0.0f;
    pid.state.antiWindupContribute = 0.0f;
    pid.state.rawOut = 0.0f;
    pid.state.output_ramp = 0.0f;
    pid.state.setpoint_ramp = 0.0f;
    pid.state.ramp_initialized = false;
    pid.state.output_ramp_initialized = false;
    pid.state.debounce_counter = 0;
}

void TemperatureController::resetIntegral() {
    pid.state.integrale = 0.0f;
    pid.state.errore_prec = 0.0f;
    pid.state.ramp_initialized = false;
}

void TemperatureController::update(float currentTemp) {
    if (!enabled) {
        output = 0.0;
        digitalWrite(pin, LOW);
        outputActive = false;
        return;
    }
    
    // Accedi a pidParams globale per manual_mode e manual_output
    extern PIDParams pidParams;
    
    // Forza l'applicazione immediata del cambio di modalità (bypassa debounce se richiesto)
    static bool lastManualMode = false;
    if (pidParams.manual_mode != lastManualMode) {
        // Cambio di modalità richiesto: forza l'applicazione immediata
        pid.state.manual_mode = pidParams.manual_mode;
        pid.state.pending_mode = pidParams.manual_mode;
        pid.state.debounce_counter = 0;
        lastManualMode = pidParams.manual_mode;
    }
    
    // Chiama PID_Mngt con i parametri appropriati
    output = PID_Mngt(
        &pid,
        currentSetpoint,         // setpoint
        currentTemp,             // measure
        0.0f,                    // reference (feedforward)
        false,                   // stop
        pidParams.manual_mode,   // manual_mode dal webserver
        false,                   // derivativeEnable - DISABILITATO (solo P)
        false,                   // awEnable - DISABILITATO (solo P)
        pidParams.manual_output  // manual_output dal webserver
    );
    
    updateOutput();
}

void TemperatureController::updateOutput() {
    unsigned long now = millis();
    unsigned long cycleElapsed = now - cycleStartTime;
    
    // Check if we need to start a new cycle
    if (cycleElapsed >= cyclePeriodMs) {
        cycleStartTime = now;
        cycleElapsed = 0;
    }
    
    // Calculate ON time for this cycle based on output percentage
    unsigned long onTime = (unsigned long)((output / 100.0) * cyclePeriodMs);
    
    // Apply minimum pulse width constraint
    if (onTime > 0 && onTime < minPulseMs) {
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
