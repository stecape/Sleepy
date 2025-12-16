#include "Output.h"
#include <Arduino.h>

static uint8_t outPin = 255;

void output_init(uint8_t pin) {
    outPin = pin;
    pinMode(outPin, OUTPUT);
}

void output_update(bool active) {
    if (outPin != 255) {
        digitalWrite(outPin, active ? HIGH : LOW);
    }
}
