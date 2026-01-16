#include "Menu.h"
#include "Timer.h"
#include <Arduino.h>

static int cursor = 0;
static bool editing = false;
static bool playing = false;
static bool finished = false;
static bool resetReq = false;
static int timer_hh = 0, timer_mm = 0, timer_ss = 0;
static PageType currentPage = PAGE_TIMER;

#define NUM_FIELDS 5
#define FIELD_HH 0
#define FIELD_MM 1
#define FIELD_SS 2
#define FIELD_PLAY 3
#define FIELD_RESET 4

// Heating page fields
#define NUM_HEATING_FIELDS 2
#define HEATING_SETPOINT 0
#define HEATING_ENABLE 1

// Header click regions (x coordinates on 160x128 display, rotation 3)
#define HEADER_TIMER_X1 0
#define HEADER_TIMER_X2 80
#define HEADER_HEATING_X1 80
#define HEADER_HEATING_X2 160
#define HEADER_Y1 0
#define HEADER_Y2 20

void menu_init() {
    cursor = 0;
    editing = false;
    playing = false;
    finished = false;
    resetReq = false;
    timer_hh = timer_mm = timer_ss = 0;
    currentPage = PAGE_TIMER;
}

void menu_move_cursor(int dir) {
    int maxFields = (currentPage == PAGE_TIMER) ? NUM_FIELDS : NUM_HEATING_FIELDS;
    cursor += dir;
    if (cursor < 0) cursor = maxFields - 1;
    if (cursor >= maxFields) cursor = 0;
}

void menu_edit_field(int dir) {
    switch (cursor) {
        case FIELD_HH:
            timer_hh += dir;
            if (timer_hh < 0) timer_hh = 99;
            if (timer_hh > 99) timer_hh = 0;
            break;
        case FIELD_MM:
            timer_mm += dir;
            if (timer_mm < 0) timer_mm = 59;
            if (timer_mm > 59) timer_mm = 0;
            break;
        case FIELD_SS:
            timer_ss += dir;
            if (timer_ss < 0) timer_ss = 59;
            if (timer_ss > 59) timer_ss = 0;
            break;
    }
}

void menu_handle_click() {
    if (currentPage == PAGE_TIMER) {
        if (cursor <= FIELD_SS) {
            editing = !editing;
        } else if (cursor == FIELD_PLAY) {
            if (!playing && (timer_hh > 0 || timer_mm > 0 || timer_ss > 0)) {
                playing = true;
                finished = false;
                timer_set(timer_hh, timer_mm, timer_ss);
                timer_start();
            } else {
                playing = false;
                timer_pause();
            }
        } else if (cursor == FIELD_RESET) {
            resetReq = true;
        }
    } else if (currentPage == PAGE_HEATING) {
        if (cursor == HEATING_SETPOINT) {
            editing = !editing;
        }
        // HEATING_ENABLE will be handled in main.cpp
    }
}

int menu_get_cursor() { return cursor; }
bool menu_is_editing() { return editing; }
void menu_set_timer(int hh, int mm, int ss) { timer_hh = hh; timer_mm = mm; timer_ss = ss; }
void menu_get_timer(int &hh, int &mm, int &ss) { hh = timer_hh; mm = timer_mm; ss = timer_ss; }
bool menu_is_playing() { return playing; }
bool menu_is_finished() { return finished; }
bool menu_reset_requested() { return resetReq; }
void menu_clear_reset() { resetReq = false; }

PageType menu_get_current_page() { 
    return currentPage; 
}

void menu_set_page(PageType page) { 
    currentPage = page;
    cursor = 0;  // Reset cursor when changing page
    editing = false;  // Exit editing mode
    
    // Force display redraw on page change
    extern void eink_force_redraw();
    eink_force_redraw();
}

bool menu_handle_header_click(int x, int y) {
    // Check if click is in header area
    if (y >= HEADER_Y1 && y <= HEADER_Y2) {
        if (x >= HEADER_TIMER_X1 && x < HEADER_TIMER_X2) {
            // Clicked on TIMER
            if (currentPage != PAGE_TIMER) {
                menu_set_page(PAGE_TIMER);
                return true;
            }
        } else if (x >= HEADER_HEATING_X1 && x < HEADER_HEATING_X2) {
            // Clicked on HEATING
            if (currentPage != PAGE_HEATING) {
                menu_set_page(PAGE_HEATING);
                return true;
            }
        }
    }
    return false;
}
