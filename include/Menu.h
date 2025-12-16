#pragma once

void menu_init();
void menu_move_cursor(int dir);
void menu_edit_field(int dir);
void menu_handle_click();
int menu_get_cursor();
bool menu_is_editing();
void menu_set_timer(int hh, int mm, int ss);
void menu_get_timer(int &hh, int &mm, int &ss);
bool menu_is_playing();
bool menu_is_finished();
bool menu_reset_requested();
void menu_clear_reset();
