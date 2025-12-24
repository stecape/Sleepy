#include <Arduino.h>
#include "Encoder.h"
#include "Menu.h"
#include "Timer.h"
#include "Output.h"
#include "EinkDisplay.h"

// Pin liberi sul D1 Mini (non in conflitto con SPI del display TFT)
// Display usa: D3(DC), D4(RST), D5(SCK), D7(MOSI), D8(CS)
// SPI usa anche: D6(MISO/GPIO12) - forzato LOW, usiamo come OUTPUT
// Encoder: polling completo
#define ENCODER_PIN_A D1  // GPIO5  - DT
#define ENCODER_PIN_B D2  // GPIO4  - CLK
#define ENCODER_BTN   D0  // GPIO16 - SW (polling)
#define OUTPUT_PIN    D6  // GPIO12 (MISO come OUTPUT)

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== Sleepy Timer Starting ===");
    
    Serial.println("Init encoder...");
    encoder_init(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_BTN);
    
    Serial.println("Init menu...");
    menu_init();
    
    Serial.println("Init timer...");
    timer_init();
    
    Serial.println("Init output...");
    output_init(OUTPUT_PIN);
    
    Serial.println("Init TFT display...");
    eink_init();
    
    Serial.println("Setup complete!");
}

void loop() {
    bool needsUpdate = false;
    static int lastHH = 0, lastMM = 0, lastSS = 0;
    static int lastCursor = 0;
    static bool lastEditing = false;
    static bool lastRunning = false;
    static bool lastFinished = false;
    
    encoder_update();
    if (encoder_was_moved()) {
        int dir = -encoder_get_direction(); // Inverti direzione per logica menu
        
        if (menu_is_editing()) {
            menu_edit_field(dir);
        } else {
            menu_move_cursor(dir);
        }
        needsUpdate = true;
    }
    if (encoder_was_clicked()) {
        menu_handle_click();
        needsUpdate = true;
    }
    encoder_reset_flags();

    timer_tick();

    // Sincronizza il menu con il timer quando è in esecuzione
    if (timer_is_running()) {
        int timer_hh, timer_mm, timer_ss;
        timer_get(timer_hh, timer_mm, timer_ss);
        menu_set_timer(timer_hh, timer_mm, timer_ss);
    }

    // Gestione reset
    if (menu_reset_requested()) {
        timer_reset();
        menu_clear_reset();
        needsUpdate = true;
    }

    // Aggiorna output in base allo stato timer
    output_update(timer_is_running() && !timer_is_finished());

    // Ottieni valori correnti
    int hh, mm, ss;
    menu_get_timer(hh, mm, ss);
    int cursor = menu_get_cursor();
    bool editing = menu_is_editing();
    bool running = timer_is_running();
    bool finished = timer_is_finished();
    
    // Controlla se qualcosa è cambiato
    if (hh != lastHH || mm != lastMM || ss != lastSS ||
        cursor != lastCursor || editing != lastEditing ||
        running != lastRunning || finished != lastFinished) {
        needsUpdate = true;
        lastHH = hh; lastMM = mm; lastSS = ss;
        lastCursor = cursor;
        lastEditing = editing;
        lastRunning = running;
        lastFinished = finished;
    }

    // Aggiorna display solo se necessario
    if (needsUpdate) {
        Serial.println("*** Display update triggered ***");
        drawMenu(cursor, editing, hh, mm, ss, running, finished);
    }

    delay(10);
}