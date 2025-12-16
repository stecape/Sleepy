#include "Timer.h"
#include <Arduino.h>

static int hh = 0, mm = 0, ss = 0;
static bool running = false;
static bool finished = false;
static unsigned long lastTick = 0;

void timer_init() {
    hh = mm = ss = 0;
    running = false;
    finished = false;
    lastTick = millis();
}

void timer_tick() {
    if (!running || finished) return;
    unsigned long now = millis();
    if (now - lastTick >= 1000) {
        lastTick = now;
        if (ss > 0) {
            ss--;
        } else {
            if (mm > 0) {
                mm--;
                ss = 59;
            } else if (hh > 0) {
                hh--;
                mm = 59;
                ss = 59;
            } else {
                running = false;
                finished = true;
            }
        }
    }
}

void timer_start() { running = true; finished = false; }
void timer_pause() { running = false; }
void timer_reset() { hh = mm = ss = 0; running = false; finished = false; }
bool timer_is_running() { return running; }
bool timer_is_finished() { return finished; }
void timer_set(int h, int m, int s) { hh = h; mm = m; ss = s; }
void timer_get(int &h, int &m, int &s) { h = hh; m = mm; s = ss; }
