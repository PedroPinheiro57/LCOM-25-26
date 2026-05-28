#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "board.h"

void board_draw(board_t *b, bool hide_ships);
void board_draw_preview(board_t *b, int col, int row,
                        uint8_t size, orientation_t orient);
void board_highlight_cell(int col, int row);
void draw_hud_place(int player, int ship_idx);
void draw_hud_attack(int player, board_t *enemy); /* enemy board for counters */
void destroy_game_sprites();
void init_game_sprites();
bool renderer_is_exploding(void);
bool renderer_explosion_finished(void);
void start_explosion(int col, int row, bool is_hit);
void update_animations();
int renderer_get_expl_col(void);
int renderer_get_expl_row(void);
