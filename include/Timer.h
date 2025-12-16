#pragma once

void timer_init();
void timer_tick();
void timer_start();
void timer_pause();
void timer_reset();
bool timer_is_running();
bool timer_is_finished();
void timer_set(int hh, int mm, int ss);
void timer_get(int &hh, int &mm, int &ss);
