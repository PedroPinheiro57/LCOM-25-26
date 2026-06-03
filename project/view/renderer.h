#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../model/board.h"
#include "../controller/game.h"
#include "sprites.h"

#define RTC_X 30
#define RTC_Y 80

#define C_EMPTY    0x1a3a5c
#define C_SHIP     0x808080
#define C_HIT      0xFF4500
#define C_MISS     0x00BFFF
#define C_SUNK     0x8B0000
#define C_VALID    0x00FF00
#define C_INVALID  0xFF0000
#define C_HOVER    0xFFFF00
#define C_REMOTE   0xFF8C00

/* sprite getters */
sprite_t* get_ship_sprite(int index);
sprite_t* get_ship_dead_sprite(int index);
animated_sprite_t* get_anim_flame(void);
animated_sprite_t* get_anim_explosion(void);

/* Draw the entire board. hide_ships=true masks CELL_SHIP cells.     */
void board_draw(board_t *b, bool hide_ships);

/* Draw the ship-placement preview (green=valid, red=invalid).       */
void board_draw_preview(board_t *b, int col, int row,
                        uint8_t size, orientation_t orient);

/* Draw the local player's attack cursor (yellow).                   */
void board_highlight_cell(int col, int row);

/* remote player */
void board_highlight_remote_cursor(int col, int row);

                                   
void draw_hud_place(int player, int ship_idx, uint32_t timer_seconds);
void draw_hud_attack(int player, board_t *enemy, uint32_t timer_seconds); /* enemy board for counters */
void draw_stopwatch(uint32_t total_seconds, uint16_t x, uint16_t y);
void init_game_sprites();
bool renderer_is_exploding(void);
bool renderer_explosion_finished(void);
void start_explosion(int col, int row, bool is_hit);
void update_animations();
int renderer_get_expl_col(void);
int renderer_get_expl_row(void);
void game_draw(const game_t *g);
void renderer_reset(void);
