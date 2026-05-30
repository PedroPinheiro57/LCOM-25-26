#pragma once
/*
 * renderer.h — Board and HUD drawing functions.
 *
 * CHANGE FROM ORIGINAL:
 *   Added board_highlight_remote_cursor() — draws the opponent's
 *   hovering cursor on the defending player's screen in a distinct
 *   orange colour, so the defender can see where the attacker aims.
 */
#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* Draw the entire board. hide_ships=true masks CELL_SHIP cells.     */
void board_draw(board_t *b, bool hide_ships);

/* Draw the ship-placement preview (green=valid, red=invalid).       */
void board_draw_preview(board_t *b, int col, int row,
                        uint8_t size, orientation_t orient);

/* Draw the local player's attack cursor (yellow).                   */
void board_highlight_cell(int col, int row);

/*
 * board_highlight_remote_cursor() — NEW
 * Draw the OPPONENT's cursor on the defending player's screen.
 * Uses a distinct orange colour so the defender can tell it apart
 * from the local cursor (yellow).
 * Called on the CLIENT only, when remote_cursor_col >= 0.
 */
void board_highlight_remote_cursor(int col, int row);

/* HUD for ship-placement phase.                                     */
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
