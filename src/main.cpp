#include <EEPROM.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include "WebServer.h"
#include "Encoder.h"
#include "Menu.h"
#include "Timer.h"
#include "Output.h"
#include "EinkDisplay.h"
#include "TemperatureController.h"
#include "NTCReader.h"

// ===== CONFIGURAZIONE con SPI SOFTWARE =====
// Display SPI: TX(SCK), RX(MOSI), D5(RES), D6(DC), D7(CS), D8(LED) - SOFTWARE
// Encoder: D0(BTN), D1(DT), D2(CLK) ✓✓✓
// Output: D3(TIMER), D4(TEMP_CTRL) ✓✓
// Input: A0(NTC) ✓
// NOTA: Serial debug DISABILITATO (TX/RX usati per display)
#define ENCODER_PIN_A D1  // GPIO5  - DT
#define ENCODER_PIN_B D2  // GPIO4  - CLK
#define ENCODER_BTN   D0  // GPIO16 - SW
#define OUTPUT_PIN_1  D4  // GPIO0  - TIMER
#define OUTPUT_PIN_2  D3  // GPIO2  - TEMP CTRL
#define OUTPUT_PIN_3  D8  // GPIO15 - LED/Extra output
#define INPUT_NTC     A0  // ADC    - NTC (futuro)
#define INPUT_NTC     A0  // ADC   - NTC (futuro)

// Global objects
TemperatureController tempController(OUTPUT_PIN_2);
NTCReader ntcReader(INPUT_NTC);

void setup() {
    // --- CONFIGURAZIONE WIFI (modifica SSID e PASSWORD!) ---
    WiFi.mode(WIFI_STA);
    // Impostazione IP statico
    IPAddress staticIP(192, 168, 2, 58);
    IPAddress gateway(192, 168, 2, 1);      // Modifica se il tuo gateway è diverso
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns(8, 8, 8, 8);
    WiFi.config(staticIP, gateway, subnet, dns);
    WiFi.begin("Boring", "SoooBoring");
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        delay(5000);
        ESP.restart();
    }

    // --- OTA SETUP ---
    ArduinoOTA.setHostname("SleepyESP8266");
    ArduinoOTA.onStart([]() {
        // Optional: azioni all'inizio OTA
    });
    ArduinoOTA.onEnd([]() {
        // Optional: azioni alla fine OTA
    });
    ArduinoOTA.onError([](ota_error_t error) {
        // Optional: gestione errori OTA
    });
    ArduinoOTA.begin();
    
    // Serial.begin(115200);  // DISABILITATO: TX/RX usati per display
    delay(1000);
    // Serial.println("\n\n=== Sleepy Timer Starting ===");
    
    // Serial.println("Init encoder...");
    encoder_init(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_BTN);
    
    // Serial.println("Init menu...");
    menu_init();
    
    // Serial.println("Init timer...");
    timer_init();
    
    // Serial.println("Init output...");
    output_init(OUTPUT_PIN_1);
    
    // Serial.println("Init TFT display...");
    eink_init();
    
    // Initialize temperature controller with default parameters
    // These can be tuned later
    tempController.setPIDParameters(10.0, 0.5, 5.0);
    tempController.setOutputLimits(0.0, 100.0);
    
    // Initialize NTC reader with Aussel NTC 10K 3950 parameters
    // Beta = 3950, R0 = 10k at 25°C, Series resistor = 10.28k
    ntcReader.setNTCParameters(10000.0, 25.0, 3950.0, 10280.0);
    
    // Setup web server
    setupWebServer();
    
    // Serial.println("Setup complete!");
}

void loop() {
    handleWebServer();
    ArduinoOTA.handle();
    bool needsUpdate = false;
    static int lastHH = 0, lastMM = 0, lastSS = 0;
    static int lastCursor = 0;
    static bool lastEditing = false;
    static bool lastRunning = false;
    static bool lastFinished = false;
    static PageType lastPage = PAGE_TIMER;
    
    // Temperature control variables
    extern PIDParams pidParams;
    static float tempActual = 22.5;
    static unsigned long lastTempRead = 0;
    static unsigned long lastTempUpdate = 0;
    static unsigned long lastDisplayUpdate = 0;
    static float lastTempSetpoint = -999.0;  // Track for display updates
    static float lastTempActual = -999.0;    // Track for display updates
    static float lastPidOutput = -999.0;     // Track for display updates
    
    // Sincronizza tempSetpoint e tempEnabled con pidParams (modificabili da webserver)
    float tempSetpoint = pidParams.setpoint;
    bool tempEnabled = pidParams.enabled;
    
    encoder_update();
    if (encoder_was_moved()) {
        // Wake up display se in sleep
        if (eink_is_sleeping()) {
            eink_wake_up();
            encoder_reset_flags();  // Ignora questo movimento, serve solo per wake
            return;
        }
        
        int dir = -encoder_get_direction(); // Inverti direzione per logica menu
        
        if (menu_is_editing()) {
            // Check which page we're on
            PageType currentPage = menu_get_current_page();
            if (currentPage == PAGE_HEATING) {
                // Edit heating page fields
                int cursor = menu_get_cursor();
                if (cursor == 0) {  // Setpoint
                    pidParams.setpoint += dir * 0.5;
                    if (pidParams.setpoint < 10.0) pidParams.setpoint = 10.0;
                    if (pidParams.setpoint > 50.0) pidParams.setpoint = 50.0;
                    tempSetpoint = pidParams.setpoint;  // Sincronizza locale
                }
            } else {
                menu_edit_field(dir);
            }
        } else {
            menu_move_cursor(dir);
        }
        needsUpdate = true;
    }
    
    if (encoder_was_long_pressed()) {
        // Wake up display se in sleep
        if (eink_is_sleeping()) {
            eink_wake_up();
            encoder_reset_flags();
            return;
        }
        
        // Long press: switch page
        PageType currentPage = menu_get_current_page();
        if (currentPage == PAGE_TIMER) {
            menu_set_page(PAGE_HEATING);
        } else {
            menu_set_page(PAGE_TIMER);
        }
        needsUpdate = true;
    } else if (encoder_was_clicked()) {
        // Wake up display se in sleep
        if (eink_is_sleeping()) {
            eink_wake_up();
            encoder_reset_flags();
            return;
        }
        
        // Normal click: handle menu actions
        PageType currentPage = menu_get_current_page();
        if (currentPage == PAGE_HEATING) {
            int cursor = menu_get_cursor();
            if (cursor == 0) {  // Setpoint field
                menu_handle_click();  // Toggle editing
            } else if (cursor == 1) {  // Enable/Disable button
                pidParams.enabled = !pidParams.enabled;
                tempEnabled = pidParams.enabled;
                if (tempEnabled) {
                    tempController.setSetpoint(tempSetpoint);
                    tempController.enable();
                } else {
                    tempController.disable();
                }
            }
        } else {
            menu_handle_click();
        }
        needsUpdate = true;
    }
    encoder_reset_flags();

    timer_tick();
    
    // Read temperature sensor periodically (every 500ms)
    unsigned long now = millis();
    if (now - lastTempRead >= 500) {
        extern PIDParams pidParams;
        tempActual = ntcReader.readTemperature() + pidParams.compensazione;
        lastTempRead = now;
    }
    
    // Update temperature controller (every 100ms for responsive control)
    if (now - lastTempUpdate >= 100) {
        tempController.update(tempActual);
        lastTempUpdate = now;
        
        // Update setpoint if changed in editing mode
        if (tempEnabled) {
            tempController.setSetpoint(tempSetpoint);
        }
    }

    // Sincronizza il menu con il timer quando è in esecuzione
    if (timer_is_running()) {
        int timer_hh, timer_mm, timer_ss;
        timer_get(timer_hh, timer_mm, timer_ss);
        menu_set_timer(timer_hh, timer_mm, timer_ss);
    }

    // Gestione reset
    if (menu_reset_requested()) {
        timer_reset();
        menu_set_timer(0, 0, 0); // Sincronizza subito il menu
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
    PageType currentPage = menu_get_current_page();
    
    // Check for periodic updates (every second) for temperature and PID output
    if (currentPage == PAGE_HEATING && (now - lastDisplayUpdate >= 1000)) {
        float pidOutput = tempController.getOutput();
        if (abs(tempActual - lastTempActual) >= 0.1 || abs(pidOutput - lastPidOutput) >= 1.0) {
            needsUpdate = true;
            lastTempActual = tempActual;
            lastPidOutput = pidOutput;
        }
        lastDisplayUpdate = now;
    }
    
    // Controlla se qualcosa è cambiato
    if (hh != lastHH || mm != lastMM || ss != lastSS ||
        cursor != lastCursor || editing != lastEditing ||
        running != lastRunning || finished != lastFinished ||
        currentPage != lastPage || tempSetpoint != lastTempSetpoint) {
        needsUpdate = true;
        lastHH = hh; lastMM = mm; lastSS = ss;
        lastCursor = cursor;
        lastEditing = editing;
        lastRunning = running;
        lastFinished = finished;
        lastPage = currentPage;
        lastTempSetpoint = tempSetpoint;
    }

    // Aggiorna display solo se necessario
    if (needsUpdate) {
        // Serial.println("*** Display update triggered ***");
        if (currentPage == PAGE_TIMER) {
            drawMenu(cursor, editing, hh, mm, ss, running, finished);
        } else if (currentPage == PAGE_HEATING) {
            float pidOutput = tempController.getOutput();
            drawHeatingPage(cursor, editing, tempSetpoint, tempActual, tempEnabled, pidOutput);
        }
    }
    
    // Controlla e gestisci sleep mode
    eink_check_sleep(tempEnabled, running);
    
    // Forza aggiornamento se appena uscito da sleep
    if (eink_needs_redraw_after_wake()) {
        needsUpdate = true;
    }

    delay(10);
}