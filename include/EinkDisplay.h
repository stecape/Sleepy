#pragma once
#include <Arduino.h>
#include "Menu.h"  // For PageType

// Rinominato ma mantiene il nome per compatibilità con il resto del codice
void eink_init();
void drawMenu(int cursor, bool editing, int hh, int mm, int ss, bool running, bool finished);
void drawHeatingPage(int cursor, bool editing, float setpoint, float actual, bool enabled, float output);
void drawHeader(PageType currentPage);  // Draw navigation header
void eink_force_redraw();  // Force complete redraw on page change
void eink_backlight_on();
void eink_backlight_off();
void eink_check_sleep(bool heatingEnabled, bool timerRunning);  // Check and handle sleep mode
void eink_wake_up();  // Wake display from sleep
bool eink_is_sleeping();  // Check if display is in sleep mode
bool eink_needs_redraw_after_wake();  // Check if display just woke up and needs redraw
