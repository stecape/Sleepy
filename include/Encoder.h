#pragma once

#include <Arduino.h>

void encoder_init(uint8_t pinA, uint8_t pinB, uint8_t btnPin);
void encoder_update();
int encoder_get_direction();
bool encoder_was_moved();
bool encoder_was_clicked();
void encoder_reset_flags();
