#include "Menu.h"
#include "Timer.h"

static int cursor = 0;
static bool editing = false;
static bool playing = false;
static bool finished = false;
static bool resetReq = false;
static int timer_hh = 0, timer_mm = 0, timer_ss = 0;

#define NUM_FIELDS 5
#define FIELD_HH 0
#define FIELD_MM 1
#define FIELD_SS 2
#define FIELD_PLAY 3
#define FIELD_RESET 4

void menu_init() {
    cursor = 0;
    editing = false;
    playing = false;
    finished = false;
    resetReq = false;
    timer_hh = timer_mm = timer_ss = 0;
}

void menu_move_cursor(int dir) {
    cursor += dir;
    if (cursor < 0) cursor = NUM_FIELDS - 1;
    if (cursor >= NUM_FIELDS) cursor = 0;
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
}

int menu_get_cursor() { return cursor; }
bool menu_is_editing() { return editing; }
void menu_set_timer(int hh, int mm, int ss) { timer_hh = hh; timer_mm = mm; timer_ss = ss; }
void menu_get_timer(int &hh, int &mm, int &ss) { hh = timer_hh; mm = timer_mm; ss = timer_ss; }
bool menu_is_playing() { return playing; }
bool menu_is_finished() { return finished; }
bool menu_reset_requested() { return resetReq; }
void menu_clear_reset() { resetReq = false; }
