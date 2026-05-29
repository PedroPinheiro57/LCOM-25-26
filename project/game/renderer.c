/*
 * renderer.c — Board and HUD rendering.
 *
 * CHANGE FROM ORIGINAL:
 *   Added board_highlight_remote_cursor() below the existing
 *   board_highlight_cell() function.  Everything else is identical.
 */

#include "renderer.h"
#include "../video/font.h"
#include "../../pedro/lab5/video.h"

/* Cell fill colours */
#define C_EMPTY    0x1a3a5c   /* dark blue — empty water             */
#define C_SHIP     0x808080   /* grey      — own ship visible        */
#define C_HIT      0xFF4500   /* orange-red — hit cell               */
#define C_MISS     0x00BFFF   /* light blue — miss cell              */
#define C_SUNK     0x8B0000   /* dark red   — sunk ship              */
#define C_VALID    0x00FF00   /* green      — valid placement preview */
#define C_INVALID  0xFF0000   /* red        — invalid placement       */
#define C_HOVER    0xFFFF00   /* yellow     — local attack cursor     */

/*
 * NEW: orange colour for the remote (opponent's) cursor.
 * Visually distinct from yellow (local cursor) and orange-red (hit).
 */
#define C_REMOTE   0xFF8C00   /* dark orange — remote cursor         */

/* ------------------------------------------------------------------ */
/* Internal: draw one cell as a filled rectangle (1px border gap)    */
/* ------------------------------------------------------------------ */
static uint8_t num_to_str(uint8_t n, char buf[3]) {
    if (n >= 10) {
        buf[0] = '0' + n / 10;
        buf[1] = '0' + n % 10;
        buf[2] = '\0';
        return 2;
    }
    buf[0] = '0' + n;
    buf[1] = '\0';
    return 1;
}

static void draw_cell(uint8_t col, uint8_t row, uint32_t color) {
    uint16_t px = BOARD_X + col * CELL_SIZE + 1;
    uint16_t py = BOARD_Y + row * CELL_SIZE + 1;
    vg_draw_rectangle_project(px, py, CELL_SIZE - 2, CELL_SIZE - 2, color);
}

/* ------------------------------------------------------------------ */
/* board_draw                                                         */
/* ------------------------------------------------------------------ */
void board_draw(board_t *b, bool hide_ships) {
    /* White outer border */
    vg_draw_rectangle_project(BOARD_X - 2, BOARD_Y - 2,
        BOARD_COLS * CELL_SIZE + 4,
        BOARD_ROWS * CELL_SIZE + 4, 0xFFFFFF);

    for (uint8_t row = 0; row < BOARD_ROWS; row++) {
        for (uint8_t col = 0; col < BOARD_COLS; col++) {
            uint32_t color;
            switch (b->grid[row][col]) {
                case CELL_SHIP:
                    /* Hide ships on the enemy board during attack phase */
                    color = hide_ships ? C_EMPTY : C_SHIP;
                    break;
                case CELL_HIT:  color = C_HIT;   break;
                case CELL_MISS: color = C_MISS;  break;
                case CELL_SUNK: color = C_SUNK;  break;
                default:        color = C_EMPTY; break;
            }
            draw_cell(col, row, color);
        }
    }

    /* Column labels A-J */
    for (uint8_t col = 0; col < BOARD_COLS; col++) {
        char label[2] = { 'A' + col, '\0' };
        draw_string(label,
            BOARD_X + col * CELL_SIZE + 15,
            BOARD_Y - 30, 0xFFFFFF, 2);
    }

    /* Row labels 1-10 */
    for (uint8_t row = 0; row < BOARD_ROWS; row++) {
        char label[3];
        if (row < 9) {
            label[0] = '1' + row;
            label[1] = '\0';
        } else {
            label[0] = '1';
            label[1] = '0';
            label[2] = '\0';
        }
        draw_string(label,
            BOARD_X - 35,
            BOARD_Y + row * CELL_SIZE + 13, 0xFFFFFF, 2);
    }
}

/* ------------------------------------------------------------------ */
/* board_draw_preview                                                 */
/* ------------------------------------------------------------------ */
void board_draw_preview(board_t *b, int col, int row,
                         uint8_t size, orientation_t orient) {
    if (col < 0 || row < 0) return;
    bool valid = board_can_place(b, col, row, size, orient);
    uint32_t color = valid ? C_VALID : C_INVALID;

    for (uint8_t i = 0; i < size; i++) {
        int c = (orient == HORIZONTAL) ? col + i : col;
        int r = (orient == HORIZONTAL) ? row     : row + i;
        if (c >= BOARD_COLS || r >= BOARD_ROWS) break;
        draw_cell((uint8_t)c, (uint8_t)r, color);
    }
}

/* ------------------------------------------------------------------ */
/* board_highlight_cell — local player's attack cursor (yellow)      */
/* ------------------------------------------------------------------ */
void board_highlight_cell(int col, int row) {
    if (col < 0 || col >= BOARD_COLS) return;
    if (row < 0 || row >= BOARD_ROWS) return;
    draw_cell((uint8_t)col, (uint8_t)row, C_HOVER);
}

/* ------------------------------------------------------------------ */
/* board_highlight_remote_cursor — opponent's cursor (orange)        */
/* ------------------------------------------------------------------ */
/*
 * NEW FUNCTION.
 *
 * On the CLIENT (defending player's screen) we draw an orange cell
 * wherever the HOST (attacking player) is hovering their cursor.
 *
 * This is the "remote cursor" feature: the defender can see the
 * attacker's intended target in near-real-time, because the host
 * sends MSG_CURSOR every timer tick (30 Hz).
 *
 * We draw this AFTER board_draw() so it always appears on top of
 * the board cells, and BEFORE the local cursor so the local cursor
 * (yellow, from board_highlight_cell) has visual priority.
 *
 * Note: if col == row == (local cursor), the yellow local cursor
 * wins because game_draw() calls board_highlight_cell() afterwards.
 */
void board_highlight_remote_cursor(int col, int row) {
    if (col < 0 || col >= BOARD_COLS) return;
    if (row < 0 || row >= BOARD_ROWS) return;
    draw_cell((uint8_t)col, (uint8_t)row, C_REMOTE);
}

/* ------------------------------------------------------------------ */
/* draw_hud_place                                                     */
/* ------------------------------------------------------------------ */
void draw_hud_place(int player, int ship_idx) {
    if (player == 1)
        draw_string("PLAYER 1 - PLACE SHIPS", 224, 20, 0x00BFFF, 2);
    else
        draw_string("PLAYER 2 - PLACE SHIPS", 224, 20, 0xFFD700, 2);

    if (ship_idx < NUM_SHIPS) {
        draw_string("PLACING:", 100, 575, 0xFFFFFF, 2);
        draw_string(SHIP_NAMES[ship_idx], 240, 575, 0x00FF00, 2);
        uint8_t size = SHIP_SIZES[ship_idx];
        for (uint8_t i = 0; i < size; i++)
            vg_draw_rectangle_project(560 + i * 20, 575, 15, 15, 0x808080);
    }

    draw_string("R=ROTATE  CLICK=PLACE", 316, 550, 0x888888, 1);
}

/* ------------------------------------------------------------------ */
/* draw_hud_attack                                                    */
/* ------------------------------------------------------------------ */
void draw_hud_attack(int player, board_t *enemy) {
    if (player == 1)
        draw_string("PLAYER 1 - YOUR TURN", 240, 20, 0x00BFFF, 2);
    else
        draw_string("PLAYER 2 - YOUR TURN", 240, 20, 0xFFD700, 2);

    draw_string("CLICK TO ATTACK", 280, 558, 0x888888, 2);

    uint8_t hits   = board_count_hits(enemy);
    uint8_t misses = board_count_misses(enemy);
    uint8_t sunk   = enemy->ships_sunk;

    char num[3];

    draw_string("HITS:", 30, 548, 0xFF4500, 2);
    num_to_str(hits, num);
    draw_string(num, 126, 548, 0xFF4500, 2);

    draw_string("MISS:", 30, 568, 0x00BFFF, 2);
    num_to_str(misses, num);
    draw_string(num, 126, 568, 0x00BFFF, 2);

    draw_string("SUNK:", 30, 588, 0xFF6666, 2);
    num_to_str(sunk, num);
    draw_string(num,  126, 588, 0xFF6666, 2);
    draw_string("/5",  142, 588, 0xFF6666, 2);
}
