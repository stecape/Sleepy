#include "NTCReader.h"

NTCReader::NTCReader(uint8_t adcPin) 
    : pin(adcPin), lastTemp(25.0),
      R0(10000.0), T0(298.15), Beta(3950.0), seriesR(10000.0),
      readIndex(0), readingSum(0.0)
{
    pinMode(pin, INPUT);
    
    // Initialize filter array
    for (int i = 0; i < FILTER_SIZE; i++) {
        readings[i] = 0.0;
    }
}

void NTCReader::setNTCParameters(float r0, float t0, float beta, float seriesResistor) {
    R0 = r0;
    T0 = t0 + 273.15;  // Convert to Kelvin
    Beta = beta;
    seriesR = seriesResistor;
}

float NTCReader::readRaw() {
    // Read ADC value (0-1023 on ESP8266)
    int adcValue = analogRead(pin);
    
    // Convert to voltage (ESP8266 ADC: 0-1V range with 1024 levels)
    float voltage = (adcValue / 1024.0) * 1.0;
    
    // Calculate NTC resistance using voltage divider
    // Assuming NTC is connected between ADC and GND, with series resistor to VCC
    // V_adc = VCC * (R_ntc / (R_series + R_ntc))
    // Solving for R_ntc: R_ntc = R_series * V_adc / (VCC - V_adc)
    
    if (voltage >= 0.99) {
        // Avoid division by zero
        return R0;  // Return nominal resistance
    }
    
    float resistance = seriesR * voltage / (1.0 - voltage);
    
    return resistance;
}

float NTCReader::resistanceToTemperature(float resistance) {
    // Steinhart-Hart equation (simplified Beta parameter equation):
    // 1/T = 1/T0 + (1/Beta) * ln(R/R0)
    // Where T is in Kelvin
    
    float steinhart = (1.0 / T0) + (1.0 / Beta) * log(resistance / R0);
    float tempKelvin = 1.0 / steinhart;
    float tempCelsius = tempKelvin - 273.15;
    
    return tempCelsius;
}

float NTCReader::readTemperature() {
    // Read resistance
    float resistance = readRaw();
    
    // Convert to temperature
    float temp = resistanceToTemperature(resistance);
    
    // Apply moving average filter
    readingSum -= readings[readIndex];
    readings[readIndex] = temp;
    readingSum += temp;
    readIndex = (readIndex + 1) % FILTER_SIZE;
    
    // Calculate average
    lastTemp = readingSum / FILTER_SIZE;
    
    return lastTemp;
}
