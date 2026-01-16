#include "Encoder.h"

static volatile int encoderPos = 0;
static volatile bool encoderMoved = false;
static volatile int encoderDir = 0;
static volatile bool encoderBtnPressed = false;
static volatile bool encoderLongPressed = false;
static volatile unsigned long btnPressTime = 0;
static uint8_t pinA, pinB, btnPin;

#define LONG_PRESS_MS 800  // 800ms for long press

// KY-040 State machine - HALF-STEP version (più comune per KY-040)
#define DIR_NONE 0x00
#define DIR_CW   0x10
#define DIR_CCW  0x20

// Half-step state table (emits event ogni mezza rotazione)
const unsigned char ttable[7][4] = {
    // 00        01           10           11
    {0x03,    0x02 | DIR_CW, 0x01,        0x00},  // 0x00
    {0x03 | DIR_CCW, 0x02,   0x01,        0x00},  // 0x01
    {0x03,    0x02,          0x01 | DIR_CCW, 0x00},  // 0x02
    {0x03,    0x02,          0x01,        0x00 | DIR_CW},  // 0x03
    {0x03,    0x02,          0x01,        0x00},  // 0x04
    {0x03,    0x02,          0x01,        0x00},  // 0x05
    {0x03,    0x02,          0x01,        0x00}   // 0x06
};

static volatile unsigned int state = 0x03; // Start state per half-step
static volatile unsigned long isrCount = 0;
static volatile unsigned long movementCount = 0;

// Debug buffer
#define DEBUG_BUF_SIZE 20
static volatile unsigned char debugPinStates[DEBUG_BUF_SIZE];
static volatile unsigned char debugStates[DEBUG_BUF_SIZE];
static volatile int debugIdx = 0;

void IRAM_ATTR encoder_isr() {
    static unsigned long lastISR = 0;
    static int accumulator = 0;
    unsigned long now = micros();
    
    // Debouncing ridotto
    if (now - lastISR < 500) return; // 0.5ms
    lastISR = now;
    
    isrCount++;
    unsigned char pinstate = (digitalRead(pinA) << 1) | digitalRead(pinB);
    unsigned char oldState = state & 0x0F;
    unsigned char newState = ttable[oldState][pinstate];
    
    // Controlla i flag di direzione
    if (newState & DIR_CW) {
        accumulator++;
    } else if (newState & DIR_CCW) {
        accumulator--;
    }
    
    // Genera evento solo ogni 2 impulsi (riduce sensibilità)
    if (accumulator >= 2) {
        encoderDir = 1;
        encoderPos++;
        encoderMoved = true;
        movementCount++;
        accumulator = 0;
    } else if (accumulator <= -2) {
        encoderDir = -1;
        encoderPos--;
        encoderMoved = true;
        movementCount++;
        accumulator = 0;
    }
    
    // Aggiorna stato
    state = newState & 0x0F;
}

void IRAM_ATTR encoder_btn_isr() {
    static unsigned long lastDebounce = 0;
    unsigned long now = millis();
    if (now - lastDebounce > 300) {
        encoderBtnPressed = true;
        lastDebounce = now;
    }
}

void encoder_init(uint8_t a, uint8_t b, uint8_t btn) {
    pinA = a; pinB = b; btnPin = btn;
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);
    pinMode(btnPin, INPUT_PULLUP);
    
    // Serial.print("Encoder: pinA=");
    // Serial.print(pinA);
    // Serial.print(" pinB=");
    // Serial.print(pinB);
    // Serial.print(" btn=");
    // Serial.println(btnPin);
    
    // Inizializza lo stato
    delay(100);
    state = 0x03; // Start state
    // Serial.print("Encoder: Initial pinstate=");
    // Serial.println((digitalRead(pinA) << 1) | digitalRead(pinB), BIN);
    
    // Serial.println("Encoder: Full polling mode (D0 doesn't support interrupts)");
}

void encoder_update() {
    // Polling più veloce - metodo Gray code semplificato
    static unsigned long lastRead = 0;
    unsigned long now = micros();
    
    if (now - lastRead < 500) return; // 0.5ms per catturare tutti gli stati
    lastRead = now;
    
    static uint8_t lastA = HIGH;
    
    uint8_t currentA = digitalRead(pinA);
    uint8_t currentB = digitalRead(pinB);
    
    // Rileva cambiamento su pin A
    if (currentA != lastA) {
        lastA = currentA;
        
        // Se A va LOW, leggi B per determinare direzione
        if (currentA == LOW) {
            if (currentB == HIGH) {
                encoderDir = 1;
                encoderPos++;
                encoderMoved = true;
                // Serial.println("CW");
            } else {
                encoderDir = -1;
                encoderPos--;
                encoderMoved = true;
                // Serial.println("CCW");
            }
        }
    }
    
    // Polling del pulsante - con supporto long press
    static bool lastBtn = HIGH;
    static unsigned long lastBtnChange = 0;
    static bool btnEventSent = false;
    static bool longPressDetected = false;
    bool btnState = digitalRead(btnPin);
    
    if (btnState != lastBtn) {
        lastBtnChange = millis();
        lastBtn = btnState;
        
        if (btnState == LOW) {
            // Button pressed - start timing
            btnPressTime = millis();
            btnEventSent = false;
            longPressDetected = false;
        } else if (btnState == HIGH) {
            // Button released - check press duration
            unsigned long pressDuration = millis() - btnPressTime;
            if (!longPressDetected && !btnEventSent && pressDuration > 50 && pressDuration < LONG_PRESS_MS) {
                // Short click (debounced and not a long press)
                encoderBtnPressed = true;
                btnEventSent = true;
            }
            longPressDetected = false;
        }
    }
    
    // Check for long press while button is held
    if (btnState == LOW && !longPressDetected) {
        if ((millis() - btnPressTime) >= LONG_PRESS_MS) {
            encoderLongPressed = true;
            longPressDetected = true;
            btnEventSent = true;  // Prevent short click on release
            // Serial.println("LONG PRESS");
        }
    }
}

int encoder_get_direction() {
    return encoderDir;
}

bool encoder_was_moved() {
    return encoderMoved;
}

bool encoder_was_clicked() {
    return encoderBtnPressed;
}

bool encoder_was_long_pressed() {
    return encoderLongPressed;
}

void encoder_reset_flags() {
    encoderMoved = false;
    encoderBtnPressed = false;
    encoderLongPressed = false;
    encoderDir = 0;
}
