#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "board.h"

void board_draw(board_t *b, bool hide_ships);
void board_draw_preview(board_t *b, int col, int row,
                        uint8_t size, orientation_t orient);
void board_highlight_cell(int col, int row);
void draw_hud_place(int player, int ship_idx);
void draw_hud_attack(int player);
