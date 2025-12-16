#include "Encoder.h"

static volatile int encoderPos = 0;
static volatile bool encoderMoved = false;
static volatile int encoderDir = 0;
static volatile bool encoderBtnPressed = false;
static uint8_t pinA, pinB, btnPin;

void IRAM_ATTR encoder_isr() {
    static uint8_t last = 0;
    uint8_t state = (digitalRead(pinA) << 1) | digitalRead(pinB);
    if (state == last) return;
    if ((last == 0b00 && state == 0b01) || (last == 0b01 && state == 0b11) ||
        (last == 0b11 && state == 0b10) || (last == 0b10 && state == 0b00)) {
        encoderDir = 1;
        encoderPos++;
    } else {
        encoderDir = -1;
        encoderPos--;
    }
    encoderMoved = true;
    last = state;
}

void IRAM_ATTR encoder_btn_isr() {
    static unsigned long lastDebounce = 0;
    unsigned long now = millis();
    if (now - lastDebounce > 200) {
        encoderBtnPressed = true;
        lastDebounce = now;
    }
}

void encoder_init(uint8_t a, uint8_t b, uint8_t btn) {
    pinA = a; pinB = b; btnPin = btn;
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);
    pinMode(btnPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pinA), encoder_isr, CHANGE);
    attachInterrupt(digitalPinToInterrupt(pinB), encoder_isr, CHANGE);
    attachInterrupt(digitalPinToInterrupt(btnPin), encoder_btn_isr, FALLING);
}

void encoder_update() {
    // No-op, ISR driven
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

void encoder_reset_flags() {
    encoderMoved = false;
    encoderBtnPressed = false;
    encoderDir = 0;
}
