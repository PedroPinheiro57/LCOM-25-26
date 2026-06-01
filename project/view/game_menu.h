#pragma once
#include <stdint.h>

#define NUM_OPTIONS 3

#define OPT_X       300
#define OPT_W       200
#define OPT_H       50
#define OPT_Y_START 250
#define OPT_GAP     70

#define COLOR_SELECTED   0xFFD700
#define COLOR_UNSELECTED 0x444444
#define COLOR_TEXT       0xFFFFFF
#define COLOR_TITLE      0x00BFFF

void menu_draw_main(int selected);
void menu_draw_pause(int selected);
void menu_draw_game_over(int winner);
void menu_draw_handover(int player);
void menu_draw_instructions(void);
int menu_mouse_hover(int x, int y);
