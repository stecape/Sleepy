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
    tft.setCursor(10, 30);
    tft.println("Ciao, Pa'!");
    
    // Disegna smile con forme geometriche
    int centerX = 80;
    int centerY = 85;
    
    // Faccia - cerchio giallo
    tft.drawCircle(centerX, centerY, 30, COLOR_HIGHLIGHT);
    tft.drawCircle(centerX, centerY, 29, COLOR_HIGHLIGHT);
    
    // Occhi - cerchi pieni gialli
    tft.fillCircle(centerX - 10, centerY - 8, 4, COLOR_HIGHLIGHT);
    tft.fillCircle(centerX + 10, centerY - 8, 4, COLOR_HIGHLIGHT);
    
    // Bocca sorridente - arco fatto con pixel (parabola verso l'alto!)
    for (int i = -12; i <= 12; i++) {
        int y = centerY + 15 - (i * i) / 30;  // Invertito per sorridere!
        tft.drawPixel(centerX + i, y, COLOR_HIGHLIGHT);
        tft.drawPixel(centerX + i, y + 1, COLOR_HIGHLIGHT);
    }
    
    delay(3000);
    
    // Serial.println("Display: Initialization complete!");
}

void drawHeader(PageType currentPage) {
    // Draw header with two buttons: TIMER | HEATING
    tft.fillRect(0, 0, 160, 20, COLOR_BG);
    
    tft.setTextSize(1);
    
    // TIMER button (left half)
    if (currentPage == PAGE_TIMER) {
        tft.fillRect(0, 0, 80, 20, COLOR_HIGHLIGHT);
        tft.setTextColor(COLOR_BG);
    } else {
        tft.setTextColor(COLOR_TEXT);
    }
    tft.setCursor(20, 6);
    tft.println("TIMER");
    
    // Separator
    tft.drawFastVLine(80, 0, 20, COLOR_TEXT);
    
    // HEATING button (right half)
    if (currentPage == PAGE_HEATING) {
        tft.fillRect(81, 0, 79, 20, COLOR_HIGHLIGHT);
        tft.setTextColor(COLOR_BG);
    } else {
        tft.setTextColor(COLOR_TEXT);
    }
    tft.setCursor(90, 6);
    tft.println("HEATING");
    
    // Bottom border
    tft.drawFastHLine(0, 20, 160, COLOR_TEXT);
}

// Static flags for forcing redraw
static bool forceTimerRedraw = true;
static bool forceHeatingRedraw = true;
static unsigned long lastRefreshTime = 0;
static uint16_t refreshCounter = 0;
static bool displaySleeping = false;
static unsigned long lastActivityTime = 0;
static bool justWokenUp = false;  // Flag per forzare ridisegno dopo wake
#define REFRESH_DEBOUNCE_MS 1000
#define CLEAR_EVERY_N_REFRESHES 100
#define SLEEP_TIMEOUT_MS 120000  // 2 minuti

void eink_force_redraw() {
    forceTimerRedraw = true;
    forceHeatingRedraw = true;
    refreshCounter = 0;  // Reset contatore per evitare clear immediato
}

bool eink_is_sleeping() {
    return displaySleeping;
}

bool eink_needs_redraw_after_wake() {
    if (justWokenUp) {
        justWokenUp = false;
        return true;
    }
    return false;
}

void eink_wake_up() {
    if (displaySleeping) {
        digitalWrite(TFT_LED, HIGH);  // Accendi backlight
        tft.fillScreen(COLOR_BG);     // Clear completo
        eink_force_redraw();
        displaySleeping = false;
        justWokenUp = true;  // Segnala che serve ridisegno completo
        lastActivityTime = millis();
    }
}

void eink_check_sleep(bool heatingEnabled, bool timerRunning) {
    unsigned long now = millis();
    
    // Se c'è attività, aggiorna timestamp
    if (heatingEnabled || timerRunning) {
        lastActivityTime = now;
        if (displaySleeping) {
            eink_wake_up();
        }
        return;
    }
    
    // Se non sleeping e timeout scaduto, vai in sleep
    if (!displaySleeping && (now - lastActivityTime > SLEEP_TIMEOUT_MS)) {
        displaySleeping = true;
        tft.fillScreen(COLOR_BG);
        digitalWrite(TFT_LED, LOW);  // Spegni backlight
    }
}

void drawMenu(int cursor, bool editing, int hh, int mm, int ss, bool running, bool finished) {
    // Ignora aggiornamenti se display in sleep
    if (displaySleeping) return;
    
    // Debounce refresh: minimo 1 secondo tra aggiornamenti
    unsigned long now = millis();
    if (now - lastRefreshTime < REFRESH_DEBOUNCE_MS) {
        return;
    }
    lastRefreshTime = now;
    lastActivityTime = now;  // Aggiorna timestamp attività
    
    // Incrementa contatore e clear periodico ogni 100 refresh
    refreshCounter++;
    if (refreshCounter >= CLEAR_EVERY_N_REFRESHES) {
        tft.fillScreen(COLOR_BG);
        forceTimerRedraw = true;
        refreshCounter = 0;
    }
    
    static int lastCursor = -1;
    static bool lastEditing = false;
    static int lastHH = -1, lastMM = -1, lastSS = -1;
    static bool lastRunning = false;
    static bool lastFinished = false;
    
    // Prima volta o dopo cambio pagina, ridisegna tutto
    if (forceTimerRedraw) {
        tft.fillScreen(COLOR_BG);
        drawHeader(PAGE_TIMER);
        forceTimerRedraw = false;
        lastCursor = -1;
        lastEditing = false;
        lastHH = lastMM = lastSS = -1;
        lastRunning = lastFinished = false;
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
    if (cursor != lastCursor || editing != lastEditing || running != lastRunning) {
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

void drawHeatingPage(int cursor, bool editing, float setpoint, float actual, bool enabled, float output) {
    // Ignora aggiornamenti se display in sleep
    if (displaySleeping) return;
    
    // Debounce refresh: minimo 1 secondo tra aggiornamenti
    unsigned long now = millis();
    if (now - lastRefreshTime < REFRESH_DEBOUNCE_MS) {
        return;
    }
    lastRefreshTime = now;
    lastActivityTime = now;  // Aggiorna timestamp attività
    
    // Incrementa contatore e clear periodico ogni 100 refresh
    refreshCounter++;
    if (refreshCounter >= CLEAR_EVERY_N_REFRESHES) {
        tft.fillScreen(COLOR_BG);
        forceHeatingRedraw = true;
        refreshCounter = 0;
    }
    
    static int lastCursor = -1;
    static bool lastEditing = false;
    static float lastSetpoint = -999;
    static float lastActual = -999;
    static bool lastEnabled = false;
    static float lastOutput = -999;
    
    // Prima volta o dopo cambio pagina, ridisegna tutto
    if (forceHeatingRedraw) {
        tft.fillScreen(COLOR_BG);
        drawHeader(PAGE_HEATING);
        
        // Labels
        tft.setTextColor(COLOR_TEXT);
        tft.setTextSize(1);
        tft.setCursor(5, 30);
        tft.println("Setpoint:");
        tft.setCursor(5, 55);
        tft.println("Actual:");
        tft.setCursor(5, 80);
        tft.println("Output:");
        
        forceHeatingRedraw = false;
        lastCursor = -1;
        lastEditing = false;
        lastSetpoint = lastActual = lastOutput = -999;
        lastEnabled = false;
    }
    
    // Update setpoint if changed
    if (setpoint != lastSetpoint || cursor != lastCursor || editing != lastEditing) {
        char buf[16];
        sprintf(buf, "%.1f", setpoint);
        
        // Clear area (larger for bigger text)
        tft.fillRect(70, 26, 70, 18, COLOR_BG);
        
        // Highlight if selected
        if (cursor == 0) {
            if (editing) {
                tft.fillRect(70, 26, 70, 18, COLOR_EDITING);
                tft.setTextColor(COLOR_TEXT);  // White text on red background
            } else {
                tft.fillRect(70, 26, 70, 18, COLOR_HIGHLIGHT);
                tft.setTextColor(COLOR_BG);
            }
        } else {
            tft.setTextColor(COLOR_TIMER);
        }
        
        tft.setTextSize(2);  // Larger text
        tft.setCursor(72, 28);
        tft.print(buf);
        tft.setTextSize(1);
        tft.print("C");
        
        lastSetpoint = setpoint;
    }
    
    // Update actual temperature if changed
    if (actual != lastActual) {
        char buf[16];
        sprintf(buf, "%.1f", actual);
        
        // Clear area (larger for bigger text)
        tft.fillRect(70, 51, 70, 18, COLOR_BG);
        
        tft.setTextColor(COLOR_TIMER);
        tft.setTextSize(2);  // Larger text
        tft.setCursor(72, 53);
        tft.print(buf);
        tft.setTextSize(1);
        tft.print("C");
        
        lastActual = actual;
    }
    
    // Update output percentage if changed
    if (output != lastOutput || enabled != lastEnabled) {
        char buf[16];
        sprintf(buf, "%.0f", output);
        
        // Clear area (larger for bigger text)
        tft.fillRect(70, 76, 70, 18, COLOR_BG);
        
        if (enabled && output > 0) {
            tft.setTextColor(COLOR_EDITING);  // Red when active
        } else {
            tft.setTextColor(COLOR_TEXT);
        }
        tft.setTextSize(2);  // Larger text
        tft.setCursor(72, 78);
        tft.print(buf);
        tft.setTextSize(1);
        tft.print("%");
        
        lastOutput = output;
    }
    
    // Enable/Disable button
    if (enabled != lastEnabled || cursor != lastCursor) {
        int itemY = 105;
        const char* label = enabled ? "DISABLE" : "ENABLE";
        
        // Clear area
        tft.fillRect(5, itemY - 2, 70, 12, COLOR_BG);
        
        if (cursor == 1) {
            tft.fillRect(5, itemY - 2, 70, 12, COLOR_HIGHLIGHT);
            tft.setTextColor(COLOR_BG);
        } else {
            tft.setTextColor(COLOR_TEXT);
        }
        
        tft.setTextSize(1);
        tft.setCursor(10, itemY);
        tft.println(label);
        
        lastEnabled = enabled;
    }
    
    lastCursor = cursor;
    lastEditing = editing;
}

void eink_backlight_on() {
    digitalWrite(TFT_LED, HIGH);
}

void eink_backlight_off() {
    digitalWrite(TFT_LED, LOW);
}