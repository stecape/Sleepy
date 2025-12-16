#include "EinkDisplay.h"
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

// Pinout tipico ESP32/Arduino, modificare se necessario
#define EINK_CS   10
#define EINK_DC   9
#define EINK_RST  8
#define EINK_BUSY 7

GxEPD2_BW<GxEPD2_213_B72, GxEPD2_213_B72::HEIGHT> display(GxEPD2_213_B72(EINK_CS, EINK_DC, EINK_RST, EINK_BUSY));

void eink_init() {
    display.init();
    display.setRotation(1);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold12pt7b);
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setCursor(10, 30);
        display.print("Sleepy Timer");
    } while (display.nextPage());
}

void drawMenu(int cursor, bool editing, int hh, int mm, int ss, bool running, bool finished) {
    display.setFullWindow();
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setFont(&FreeMonoBold12pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(10, 30);
        display.print("Timer:");
        char buf[16];
        sprintf(buf, "%02d:%02d:%02d", hh, mm, ss);
        display.setCursor(10, 60);
        display.print(buf);
        display.setFont(&FreeMonoBold9pt7b);
        // Evidenziazione campo selezionato
        int y[5] = {90, 110, 130, 150, 170};
        const char* labels[5] = {"hh", "mm", "ss", running ? "Pause" : "Play", "Reset"};
        for (int i = 0; i < 5; ++i) {
            if (cursor == i) {
                display.fillRect(5, y[i] - 15, 80, 20, GxEPD_BLACK);
                display.setTextColor(GxEPD_WHITE);
            } else {
                display.setTextColor(GxEPD_BLACK);
            }
            display.setCursor(10, y[i]);
            display.print(labels[i]);
        }
        // Stato
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(100, 190);
        if (finished) display.print("FINE");
        else if (running) display.print("RUN");
        else display.print("STOP");
    } while (display.nextPage());
}
