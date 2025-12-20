/*
#include "EinkDisplay.h"
#include "EPD_2in13g.h"
#include <stdio.h>

// Pinout per ESP8266 D1 Mini (pin sicuri per boot)
#define EINK_CS   D8  // GPIO15
#define EINK_DC   D1  // GPIO5  (era D3/GPIO0 - conflitto boot!)
#define EINK_RST  D6  // GPIO12 (era D4/GPIO2 - conflitto boot!)
#define EINK_BUSY D2  // GPIO4

EPD_2in13g epd;
unsigned char imageBuffer[EPD_2IN13G_WIDTH / 4 * EPD_2IN13G_HEIGHT];

void eink_init() {
    Serial.println("Display: Initializing EPD...");
    epd.init(EINK_CS, EINK_DC, EINK_RST, EINK_BUSY);
    
    Serial.println("Display: Test 1 - Clear to BLACK...");
    epd.clear(EPD_2IN13G_BLACK);
    delay(3000);
    
    Serial.println("Display: Test 2 - Clear to RED...");
    epd.clear(EPD_2IN13G_RED);
    delay(3000);
    
    Serial.println("Display: Test 3 - Clear to YELLOW...");
    epd.clear(EPD_2IN13G_YELLOW);
    delay(3000);
    
    Serial.println("Display: Test 4 - Clear to WHITE...");
    epd.clear(EPD_2IN13G_WHITE);
    delay(2000);
    
    Serial.println("Display: Preparing buffer...");
    memset(imageBuffer, 0xFF, sizeof(imageBuffer));
    
    Serial.print("Display: Buffer size = ");
    Serial.print(sizeof(imageBuffer));
    Serial.print(" bytes, expected = ");
    Serial.print(EPD_2IN13G_WIDTH / 4 * EPD_2IN13G_HEIGHT);
    Serial.println(" bytes");
    
    Serial.println("Display: Drawing text...");
    epd.drawString(imageBuffer, 10, 10, "Sleepy Timer", EPD_2IN13G_BLACK);
    
    Serial.println("Display: Updating display...");
    epd.display(imageBuffer);
    
    Serial.println("Display: Done!");
    delay(2000);
}

void drawMenu(int cursor, bool editing, int hh, int mm, int ss, bool running, bool finished) {
    // Pulisce buffer
    memset(imageBuffer, 0xFF, sizeof(imageBuffer));
    
    // Timer label
    epd.drawString(imageBuffer, 5, 10, "Timer:", EPD_2IN13G_BLACK);
    
    // Timer value
    char buf[16];
    sprintf(buf, "%02d:%02d:%02d", hh, mm, ss);
    epd.drawString(imageBuffer, 5, 30, buf, EPD_2IN13G_RED);
    
    // Menu options
    int y[5] = {60, 80, 100, 120, 140};
    const char* labels[5] = {"hh", "mm", "ss", running ? "Pause" : "Play", "Reset"};
    
    for (int i = 0; i < 5; ++i) {
        if (cursor == i) {
            // Evidenzia selezione con rettangolo giallo
            epd.drawRectangle(imageBuffer, 3, y[i] - 2, 60, y[i] + 12, EPD_2IN13G_YELLOW, true);
            epd.drawString(imageBuffer, 5, y[i], labels[i], EPD_2IN13G_BLACK);
        } else {
            epd.drawString(imageBuffer, 5, y[i], labels[i], EPD_2IN13G_BLACK);
        }
    }
    
    // Status
    if (finished) {
        epd.drawString(imageBuffer, 5, 170, "FINE", EPD_2IN13G_RED);
    } else if (running) {
        epd.drawString(imageBuffer, 5, 170, "RUN", EPD_2IN13G_BLACK);
    } else {
        epd.drawString(imageBuffer, 5, 170, "STOP", EPD_2IN13G_BLACK);
    }
    
    // Aggiorna display
    epd.display(imageBuffer);
}

*/