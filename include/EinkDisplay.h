#pragma once
#include <Arduino.h>

// Rinominato ma mantiene il nome per compatibilità con il resto del codice
void eink_init();
void drawMenu(int cursor, bool editing, int hh, int mm, int ss, bool running, bool finished);
void eink_backlight_on();
void eink_backlight_off();
