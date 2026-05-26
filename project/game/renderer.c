#include "renderer.h"
#include "../video/font.h"
#include "../../pedro/lab5/video.h"

/* cell colors */
#define C_EMPTY    0x1a3a5c
#define C_SHIP     0x808080
#define C_HIT      0xFF4500
#define C_MISS     0x00BFFF
#define C_SUNK     0x8B0000
#define C_VALID    0x00FF00
#define C_INVALID  0xFF0000
#define C_HOVER    0xFFFF00

static void draw_cell(uint8_t col, uint8_t row, uint32_t color) {
  uint16_t px = BOARD_X + col * CELL_SIZE + 1;
  uint16_t py = BOARD_Y + row * CELL_SIZE + 1;
  vg_draw_rectangle_project(px, py, CELL_SIZE - 2, CELL_SIZE - 2, color);
}

void board_draw(board_t *b, bool hide_ships) {
  /* outer border */
  vg_draw_rectangle_project(BOARD_X - 2, BOARD_Y - 2,
    BOARD_COLS * CELL_SIZE + 4, BOARD_ROWS * CELL_SIZE + 4, 0xFFFFFF);

  for (uint8_t row = 0; row < BOARD_ROWS; row++) {
    for (uint8_t col = 0; col < BOARD_COLS; col++) {
      uint32_t color;
      switch (b->grid[row][col]) {
        case CELL_SHIP:  color = hide_ships ? C_EMPTY : C_SHIP; break;
        case CELL_HIT:   color = C_HIT;   break;
        case CELL_MISS:  color = C_MISS;  break;
        case CELL_SUNK:  color = C_SUNK;  break;
        default:         color = C_EMPTY; break;
      }
      draw_cell(col, row, color);
    }
  }

  /* column labels A-J */
  for (uint8_t col = 0; col < BOARD_COLS; col++) {
    char label[2] = { 'A' + col, '\0' };
    draw_string(label, BOARD_X + col * CELL_SIZE + 15, BOARD_Y - 30, 0xFFFFFF, 2);
  }

  /* row labels 1-10 */
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
    draw_string(label, BOARD_X - 35, BOARD_Y + row * CELL_SIZE + 13, 0xFFFFFF, 2);
  }
}

void board_draw_preview(board_t *b, int col, int row,
                         uint8_t size, orientation_t orient) {
  if (col < 0 || row < 0) return;
  bool valid = board_can_place(b, col, row, size, orient);
  uint32_t color = valid ? C_VALID : C_INVALID;

  for (uint8_t i = 0; i < size; i++) {
    int c = (orient == HORIZONTAL) ? col + i : col;
    int r = (orient == HORIZONTAL) ? row     : row + i;
    if (c >= BOARD_COLS || r >= BOARD_ROWS) break;
    draw_cell(c, r, color);
  }
}

void board_highlight_cell(int col, int row) {
  if (col < 0 || col >= BOARD_COLS) return;
  if (row < 0 || row >= BOARD_ROWS) return;
  draw_cell(col, row, C_HOVER);
}

void draw_hud_place(int player, int ship_idx) {
  /* "PLAYER X - PLACE SHIPS" = 22 * 8 * scale2 = 352px  → x = (800-352)/2 = 224 */
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

  /* "R=ROTATE  CLICK=PLACE" = 21 * 8 * scale1 = 168px  → x = (800-168)/2 = 316 */
  draw_string("R=ROTATE  CLICK=PLACE", 316, 550, 0x888888, 1);
}

void draw_hud_attack(int player) {
  /* "PLAYER X - YOUR TURN" = 20 * 8 * scale2 = 320px  → x = (800-320)/2 = 240 */
  if (player == 1)
    draw_string("PLAYER 1 - YOUR TURN", 240, 20, 0x00BFFF, 2);
  else
    draw_string("PLAYER 2 - YOUR TURN", 240, 20, 0xFFD700, 2);

  /* "CLICK TO ATTACK" = 15 * 8 * scale2 = 240px  → x = (800-240)/2 = 280 */
  draw_string("CLICK TO ATTACK", 280, 558, 0x888888, 2);
}
