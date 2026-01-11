#include "EinkDisplay.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Pin definitions per ESP8266 D1 Mini
// CONFIGURAZIONE con SPI SOFTWARE:
// - CS: D7 (GPIO13) - Chip Select
// - RST: D5 (GPIO14) - Reset
// - DC: D6 (GPIO12) - Data/Command
// - SCK: TX (GPIO1) - SCK (software)
// - MOSI: RX (GPIO3) - MOSI (software)
// - LED: resistenza 5k + VCC (sempre acceso)
// 
// Cablaggio display:
// Pin3 SCK → TX
// Pin4 MOSI → RX
// Pin5 RES → D5
// Pin6 DC → D6
// Pin7 CS → D7
// Pin8 LED → D8 (MPS A14 Darlington)
// 
// NOTA: SPI SOFTWARE
#define TFT_CS     D7  // Chip Select - GPIO13
#define TFT_RST    D5  // Reset - GPIO14
#define TFT_DC     D6  // Data/Command - GPIO12
#define TFT_MOSI   RX  // MOSI - GPIO3 (software)
#define TFT_SCK    TX  // SCK - GPIO1 (software)
#define TFT_LED    D8  // LED backlight - GPIO15

// Display object - Costruttore SPI software: (CS, DC, MOSI, SCLK, RST)
static Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);

// Colori per il display TFT
#define COLOR_BG        ST77XX_BLACK
#define COLOR_TEXT      ST77XX_WHITE
#define COLOR_HIGHLIGHT ST77XX_YELLOW
#define COLOR_EDITING   ST77XX_RED
#define COLOR_TIMER     ST77XX_CYAN

void eink_init() {
    // Serial.println("Display: Initializing TFT ST7735...");
    
    // Inizializza LED backlight
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);  // Accendi backlight
    // Serial.println("Display: LED backlight ON");
    
    // Inizializza il display
    // Serial.println("Display: Calling initR()...");
    tft.initR(INITR_BLACKTAB);
    // Serial.println("Display: initR() complete");
    
    // Serial.println("Display: Setting rotation...");
    tft.setRotation(3); // Landscape mode invertito (160x128)
    
    // Serial.println("Display: Clearing screen...");
    tft.fillScreen(ST77XX_RED);  // Prova con rosso per vedere se cambia qualcosa
    delay(1000);
    tft.fillScreen(ST77XX_GREEN);
    delay(1000);
    tft.fillScreen(COLOR_BG);
    
    // Schermata iniziale
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(2);
    tft.setCursor(20, 40);
    tft.println("Sleepy");
    tft.setCursor(20, 60);
    tft.println("Timer");
    
    tft.setTextColor(COLOR_EDITING);
    tft.setTextSize(1);
    tft.setCursor(20, 90);
    tft.println("Ready!");
    
    delay(1000);
    
    // Serial.println("Display: Initialization complete!");
}

void drawMenu(int cursor, bool editing, int hh, int mm, int ss, bool running, bool finished) {
    static int lastCursor = -1;
    static bool lastEditing = false;
    static int lastHH = -1, lastMM = -1, lastSS = -1;
    static bool lastRunning = false;
    static bool lastFinished = false;
    static bool firstDraw = true;
    
    // Prima volta ridisegna tutto
    if (firstDraw) {
        tft.fillScreen(COLOR_BG);
        
        // Titolo
        tft.setTextColor(COLOR_TEXT);
        tft.setTextSize(1);
        tft.setCursor(5, 5);
        tft.println("TIMER");
        
        firstDraw = false;
    }
    
    // Aggiorna timer solo se cambiato
    if (hh != lastHH || mm != lastMM || ss != lastSS) {
        char buf[16];
        sprintf(buf, "%02d:%02d:%02d", hh, mm, ss);
        
        // Cancella area timer
        tft.fillRect(20, 25, 96, 16, COLOR_BG);
        
        // Ridisegna timer
        tft.setTextColor(COLOR_TIMER);
        tft.setTextSize(2);
        tft.setCursor(20, 25);
        tft.println(buf);
        
        lastHH = hh; lastMM = mm; lastSS = ss;
    }
    
    // Menu items - ridisegna solo se cursore o editing è cambiato
    if (cursor != lastCursor || editing != lastEditing || running != lastRunning || firstDraw) {
        const char* labels[5] = {"HH", "MM", "SS", "START", "RESET"};
        
        // Aggiorna label START/PAUSE
        if (running) {
            labels[3] = "PAUSE";
        }
        
        tft.setTextSize(1);
        
        // Ridisegna tutte le voci
        for (int i = 0; i < 5; i++) {
            int itemY = 55 + (i * 14);
            bool isSelected = (i == cursor);
            
            // Cancella l'area
            tft.fillRect(5, itemY - 2, 60, 12, COLOR_BG);
            
            if (isSelected) {
                // Evidenzia con sfondo colorato
                if (editing && i < 3) {
                    // In modalità editing usa sfondo rosso
                    tft.fillRect(5, itemY - 2, 60, 12, COLOR_EDITING);
                    tft.setTextColor(COLOR_BG);
                } else {
                    // Selezione normale con sfondo giallo
                    tft.fillRect(5, itemY - 2, 60, 12, COLOR_HIGHLIGHT);
                    tft.setTextColor(COLOR_BG);
                }
            } else {
                tft.setTextColor(COLOR_TEXT);
            }
            
            tft.setCursor(10, itemY);
            tft.println(labels[i]);
        }
        
        lastCursor = cursor;
        lastEditing = editing;
    }
    
    // Status line - aggiorna solo se cambiato
    if (running != lastRunning || finished != lastFinished) {
        int yPos = 115;
        
        // Cancella area status
        tft.fillRect(90, yPos, 70, 10, COLOR_BG);
        
        tft.setTextSize(1);
        if (finished) {
            tft.setTextColor(COLOR_EDITING);
            tft.setCursor(90, yPos);
            tft.println("FINISHED!");
        } else if (running) {
            tft.setTextColor(COLOR_TEXT);
            tft.setCursor(90, yPos);
            tft.println("Running...");
        } else {
            tft.setTextColor(COLOR_TEXT);
            tft.setCursor(90, yPos);
            tft.println("Stopped");
        }
        
        lastRunning = running;
        lastFinished = finished;
    }
}

void eink_backlight_on() {
    digitalWrite(TFT_LED, HIGH);
}

void eink_backlight_off() {
    digitalWrite(TFT_LED, LOW);
}