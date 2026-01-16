#pragma once

#include <Arduino.h>

class NTCReader {
public:
    NTCReader(uint8_t adcPin);
    
    // Configuration
    void setNTCParameters(float r0, float t0, float beta, float seriesResistor);
    
    // Reading
    float readTemperature();  // Returns temperature in Celsius
    float getLastReading() const { return lastTemp; }
    
private:
    uint8_t pin;
    float lastTemp;
    
    // NTC parameters (Steinhart-Hart equation via Beta parameter)
    float R0;           // Resistance at T0 (typically 10k at 25°C)
    float T0;           // Reference temperature in Kelvin (typically 298.15K = 25°C)
    float Beta;         // Beta coefficient (typically 3950)
    float seriesR;      // Series resistor value (typically 10k)
    
    // Filtering
    static const int FILTER_SIZE = 10;
    float readings[FILTER_SIZE];
    int readIndex;
    float readingSum;
    
    float readRaw();
    float resistanceToTemperature(float resistance);
};
