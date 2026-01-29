#pragma once
#include <ESP8266WebServer.h>

void setupWebServer();
void handleWebServer();
void loadPIDParamsFromEEPROM();
void savePIDParamsToEEPROM();

// Struttura parametri PID avanzati (da Forno, adattata)
struct PIDParams {
    float Kp;
    float Gp;
    float Ti;
    float Td;
    float Taw;
    float out_min;
    float out_max;
    float ref_min;
    float ref_max;
    float gradiente;
    float output_gradient;
    float setpoint;
    float manual_output;
    float compensazione;
    unsigned long cycle_period_ms;  // Periodo PWM in millisecondi
    unsigned long min_pulse_ms;     // Impulso minimo in millisecondi
    bool enabled;                   // Start/Stop controllo temperatura
    bool manual_mode;               // Modalità manuale/automatica
};

extern PIDParams pidParams;
extern ESP8266WebServer server;
