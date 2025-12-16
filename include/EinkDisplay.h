#pragma once
#include <GxEPD2_BW.h>

void eink_init();
void drawMenu(int cursor, bool editing, int hh, int mm, int ss, bool running, bool finished);
