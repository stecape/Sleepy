#include <Arduino.h>
#include "Encoder.h"
#include "Menu.h"
#include "Timer.h"

#include "Output.h"
#include "EinkDisplay.h"

#define ENCODER_PIN_A 2
#define ENCODER_PIN_B 3
#define ENCODER_BTN   4
#define OUTPUT_PIN    5

void setup() {
    encoder_init(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_BTN);
    menu_init();
    timer_init();
    output_init(OUTPUT_PIN);
    eink_init();
}

void loop() {
    encoder_update();
    if (encoder_was_moved()) {
        int dir = encoder_get_direction();
        if (menu_is_editing()) {
            menu_edit_field(dir);
        } else {
            menu_move_cursor(dir);
        }
    }
    if (encoder_was_clicked()) {
        menu_handle_click();
    }
    encoder_reset_flags();

    timer_tick();

    // Gestione reset
    if (menu_reset_requested()) {
        timer_reset();
        menu_clear_reset();
    }

    // Aggiorna output in base allo stato timer
    output_update(timer_is_running() && !timer_is_finished());


    int hh, mm, ss;
    menu_get_timer(hh, mm, ss);
    drawMenu(
        menu_get_cursor(),
        menu_is_editing(),
        hh, mm, ss,
        timer_is_running(),
        timer_is_finished()
    );

    delay(10);
}