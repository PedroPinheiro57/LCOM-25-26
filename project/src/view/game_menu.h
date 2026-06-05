/**
 * @file game_menu.h
 * @brief Menu and overlay rendering for all non-gameplay screens.
 *
 * Provides drawing routines for the main menu, pause menu, game-over
 * screen, player-handover overlay, and the instructions screen, together
 * with hit-testing helpers that map a mouse position to the highlighted
 * menu option.
 *
 * Layout constants define where the option buttons are rendered so that
 * the same values can be shared between the draw and hit-test routines.
 */

#pragma once
#include <stdint.h>

/** @brief Number of selectable options in the main menu and pause menu. */
#define NUM_OPTIONS 4

/** @brief X pixel coordinate of the left edge of each menu option box. */
#define OPT_X       275
/** @brief Width in pixels of each menu option box. */
#define OPT_W       250
/** @brief Height in pixels of each menu option box. */
#define OPT_H       50
/** @brief Y pixel coordinate of the top edge of the first option box. */
#define OPT_Y_START 250
/** @brief Vertical gap in pixels between consecutive option boxes. */
#define OPT_GAP     70

/** @brief Colour used to highlight the currently selected option (gold). */
#define COLOR_SELECTED   0xFFD700
/** @brief Colour used for unselected option boxes (dark grey). */
#define COLOR_UNSELECTED 0x444444
/** @brief Colour used for option label text (white). */
#define COLOR_TEXT       0xFFFFFF
/** @brief Colour used for the title / header text (sky blue). */
#define COLOR_TITLE      0x00BFFF

/**
 * @brief Draws the main menu screen.
 *
 * Renders the game title, the three menu options (Play / Instructions /
 * Exit), and highlights the option at index @p selected.
 *
 * @param selected Zero-based index of the currently highlighted option
 *                 (0 = Play, 1 = Instructions, 2 = Exit).
 */
void menu_draw_main(int selected);

/**
 * @brief Draws the in-game pause menu overlay.
 *
 * Renders the pause menu options (Resume / Restart / Exit) on top of
 * the current game view, highlighting the option at index @p selected.
 *
 * @param selected Zero-based index of the currently highlighted option
 *                 (0 = Resume, 1 = Restart, 2 = Exit).
 */
void menu_draw_pause(int selected);

/**
 * @brief Draws the game-over screen.
 *
 * Announces which player won and offers a prompt to return to the main
 * menu.
 *
 * @param winner Winning player index (1 or 2).
 */
void menu_draw_game_over(int winner);

/**
 * @brief Draws the player-handover transition screen.
 *
 * Displayed between ship-placement phases to ask the current player to
 * hand the keyboard/mouse to the next player.
 *
 * @param player Index of the player who should now take control (1 or 2).
 */
void menu_draw_handover(int player);

/**
 * @brief Draws the instructions / help screen.
 *
 * Renders a static page explaining the game rules and controls.
 */
void menu_draw_instructions(void);

/**
 * @brief Returns the index of the main-menu option under the given cursor.
 *
 * Performs a bounding-box hit test against the option rectangles defined
 * by @ref OPT_X, @ref OPT_Y_START, @ref OPT_W, @ref OPT_H, and
 * @ref OPT_GAP.
 *
 * @param x Cursor X position in screen pixels.
 * @param y Cursor Y position in screen pixels.
 * @return Zero-based option index (0–@ref NUM_OPTIONS − 1), or @c -1 if
 *         the cursor is not over any option.
 */
int menu_mouse_hover(int x, int y);

/**
 * @brief Returns the index of the pause-menu option under the given cursor.
 *
 * Identical layout logic to @ref menu_mouse_hover but applied to the
 * pause menu option positions.
 *
 * @param x Cursor X position in screen pixels.
 * @param y Cursor Y position in screen pixels.
 * @return Zero-based option index (0–@ref NUM_OPTIONS − 1), or @c -1 if
 *         the cursor is not over any option.
 */
int menu_pause_hover(int x, int y);
