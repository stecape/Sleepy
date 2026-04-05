#include "Encoder.h"

static volatile bool encoderMoved = false;
static volatile int encoderDir = 0;
static volatile bool encoderBtnPressed = false;
static volatile bool encoderLongPressed = false;
static volatile unsigned long btnPressTime = 0;
static uint8_t pinA, pinB, btnPin;

#define LONG_PRESS_MS 800

void encoder_init(uint8_t a, uint8_t b, uint8_t btn) {
    pinA = a; pinB = b; btnPin = btn;
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);
    pinMode(btnPin, INPUT_PULLUP);
    delay(100);
}

void encoder_update() {
    static unsigned long lastRead = 0;
    unsigned long now = micros();

    if (now - lastRead < 500) return;
    lastRead = now;

    static uint8_t lastA = HIGH;

    uint8_t currentA = digitalRead(pinA);
    uint8_t currentB = digitalRead(pinB);

    if (currentA != lastA) {
        lastA = currentA;
        if (currentA == LOW) {
            if (currentB == HIGH) {
                encoderDir = 1;
                encoderMoved = true;
            } else {
                encoderDir = -1;
                encoderMoved = true;
            }
        }
    }

    // Polling del pulsante con supporto long press
    static bool lastBtn = HIGH;
    static bool btnEventSent = false;
    static bool longPressDetected = false;
    bool btnState = digitalRead(btnPin);

    if (btnState != lastBtn) {
        lastBtn = btnState;

        if (btnState == LOW) {
            btnPressTime = millis();
            btnEventSent = false;
            longPressDetected = false;
        } else if (btnState == HIGH) {
            unsigned long pressDuration = millis() - btnPressTime;
            if (!longPressDetected && !btnEventSent && pressDuration > 50 && pressDuration < LONG_PRESS_MS) {
                encoderBtnPressed = true;
                btnEventSent = true;
            }
            longPressDetected = false;
        }
    }

    if (btnState == LOW && !longPressDetected) {
        if ((millis() - btnPressTime) >= LONG_PRESS_MS) {
            encoderLongPressed = true;
            longPressDetected = true;
            btnEventSent = true;
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
