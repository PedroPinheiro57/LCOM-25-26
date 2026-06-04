/**
 * @file renderer.h
 * @brief Board and HUD rendering, sprite management, and explosion effects.
 *
 * Owns the lifetime of all in-game sprite and animated-sprite objects and
 * exposes the drawing routines used by @ref game_draw to compose each frame.
 *
 * The renderer maintains a small state machine for the post-attack explosion
 * animation: @ref start_explosion kicks it off, @ref update_animations
 * advances it each tick, and @ref renderer_explosion_finished signals when
 * the sequence is complete so the game can resume input.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../model/board.h"
#include "../controller/game.h"
#include "sprites.h"

/** @brief X pixel coordinate where the RTC clock string is drawn. */
#define RTC_X 650
/** @brief Y pixel coordinate where the RTC clock string is drawn. */
#define RTC_Y 20

/** @brief Cell fill colour for an empty (unattacked) water cell. */
#define C_EMPTY    0x1a3a5c
/** @brief Cell fill colour for a cell containing an un-hit ship segment. */
#define C_SHIP     0x808080
/** @brief Cell fill colour for a cell that has been hit. */
#define C_HIT      0xFF4500
/** @brief Cell fill colour for a cell where a shot missed (open water). */
#define C_MISS     0x00BFFF
/** @brief Cell fill colour for a cell belonging to a fully sunk ship. */
#define C_SUNK     0x8B0000
/** @brief Preview overlay colour indicating a valid ship placement. */
#define C_VALID    0x00FF00
/** @brief Preview overlay colour indicating an invalid ship placement. */
#define C_INVALID  0xFF0000
/** @brief Highlight colour for the local player's attack cursor. */
#define C_HOVER    0xFFFF00
/** @brief Highlight colour for the remote player's cursor. */
#define C_REMOTE   0xFF8C00

/**
 * @brief Returns the live-ship sprite for the given ship type index.
 *
 * @param index Ship type index (0–@ref NUM_SHIPS − 1); matches @ref SHIP_SIZES.
 * @return Pointer to the corresponding @ref sprite_t.
 */
sprite_t *get_ship_sprite(int index);

/**
 * @brief Returns the destroyed-ship sprite for the given ship type index.
 *
 * Displayed in place of @ref get_ship_sprite when a ship has been sunk.
 *
 * @param index Ship type index (0–@ref NUM_SHIPS − 1).
 * @return Pointer to the corresponding @ref sprite_t.
 */
sprite_t *get_ship_dead_sprite(int index);

/**
 * @brief Returns the animated flame sprite used for burning ships.
 *
 * @return Pointer to the shared @ref animated_sprite_t.
 */
animated_sprite_t *get_anim_flame(void);

/**
 * @brief Returns the animated explosion sprite used after a successful hit.
 *
 * @return Pointer to the shared @ref animated_sprite_t.
 */
animated_sprite_t *get_anim_explosion(void);

/**
 * @brief Draws the complete board state for one player.
 *
 * Iterates every cell and renders the appropriate colour or ship sprite.
 * When @p hide_ships is @c true, @ref CELL_SHIP cells are drawn as
 * @ref C_EMPTY so the opponent cannot see where ships are located.
 *
 * @param b          Board to draw.
 * @param hide_ships Pass @c true when drawing the opponent's board
 *                   during the attack phase.
 */
void board_draw(board_t *b, bool hide_ships);

/**
 * @brief Draws a ghost preview of the ship currently being placed.
 *
 * Fills the cells the ship would occupy with @ref C_VALID (green) if the
 * placement is legal, or @ref C_INVALID (red) if it is not.
 *
 * @param b      Player's board (used to check for collisions).
 * @param col    Grid column of the ship's top-left cell.
 * @param row    Grid row of the ship's top-left cell.
 * @param size   Length of the ship in cells.
 * @param orient Orientation of the ship being placed.
 */
void board_draw_preview(board_t *b, int col, int row,
                        uint8_t size, orientation_t orient);

/**
 * @brief Highlights the cell under the local player's attack cursor.
 *
 * Draws a @ref C_HOVER (yellow) overlay on the specified cell.
 *
 * @param col Grid column of the cursor.
 * @param row Grid row of the cursor.
 */
void board_highlight_cell(int col, int row);

/**
 * @brief Highlights the cell reported by the remote player's cursor.
 *
 * Draws a @ref C_REMOTE (orange) overlay on the specified cell so the
 * local player can see where the opponent is hovering.
 *
 * @param col Grid column of the remote cursor.
 * @param row Grid row of the remote cursor.
 */
void board_highlight_remote_cursor(int col, int row);

/**
 * @brief Draws the HUD overlay shown during the ship-placement phase.
 *
 * Displays which player is placing and which ship is currently being
 * positioned (name and remaining ships).
 *
 * @param player    Active player index (1 or 2).
 * @param ship_idx  Index of the ship being placed (0–@ref NUM_SHIPS − 1).
 */
void draw_hud_place(int player, int ship_idx);

/**
 * @brief Draws the HUD overlay shown during the attack phase.
 *
 * Displays the active player's turn indicator together with hit and miss
 * counters derived from the enemy board.
 *
 * @param player Active attacking player index (1 or 2).
 * @param enemy  Pointer to the opponent's board (used to count hits/misses).
 */
void draw_hud_attack(int player, board_t *enemy);

/**
 * @brief Loads all game sprite sheets from their XPM sources into memory.
 *
 * Must be called once during game initialisation, before any draw call
 * that references ship or animation sprites.
 */
void init_game_sprites(void);

/**
 * @brief Returns @c true while an explosion animation is in progress.
 *
 * The game controller polls this to suppress input during the animation.
 */
bool renderer_is_exploding(void);

/**
 * @brief Returns @c true once a running explosion animation has played
 *        through all of its frames.
 *
 * Transitions to @c false automatically on the next call to
 * @ref start_explosion.
 */
bool renderer_explosion_finished(void);

/**
 * @brief Starts a hit or miss explosion animation at the given grid cell.
 *
 * Positions the appropriate animated sprite over the cell and resets the
 * frame counter.  The animation advances each tick via @ref update_animations.
 *
 * @param col    Grid column of the attacked cell.
 * @param row    Grid row of the attacked cell.
 * @param is_hit @c true to play the hit/explosion effect;
 *               @c false to play the water-splash miss effect.
 */
void start_explosion(int col, int row, bool is_hit);

/**
 * @brief Advances all active animation frame counters by one tick.
 *
 * Should be called once per timer tick from the game controller.
 */
void update_animations(void);

/**
 * @brief Returns the grid column of the currently active explosion.
 *
 * Only meaningful while @ref renderer_is_exploding returns @c true.
 *
 * @return Grid column index.
 */
int renderer_get_expl_col(void);

/**
 * @brief Returns the grid row of the currently active explosion.
 *
 * Only meaningful while @ref renderer_is_exploding returns @c true.
 *
 * @return Grid row index.
 */
int renderer_get_expl_row(void);

/**
 * @brief Renders the complete game view for the current state.
 *
 * Dispatches to the appropriate sub-renderers (board, HUD, menus,
 * overlays) based on @c g->tag.  Should be called once per tick after
 * all state updates are complete.
 *
 * @param g Pointer to the current (read-only) game state.
 */
void game_draw(const game_t *g);

/**
 * @brief Resets all renderer-side state to its initial values.
 *
 * Clears any in-progress explosion, resets animation counters, and
 * returns the renderer to a clean slate.  Call before starting a new game.
 */
void renderer_reset(void);
