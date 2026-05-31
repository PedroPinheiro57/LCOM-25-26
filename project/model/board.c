#include "board.h"
#include <string.h>

void board_init(board_t *b) {
  memset(b->grid, CELL_EMPTY, sizeof(b->grid));
  memset(b->ships, 0, sizeof(b->ships));
  b->ships_placed = 0;
  b->ships_sunk   = 0;
}

bool board_can_place(board_t *b, uint8_t col, uint8_t row,
                     uint8_t size, orientation_t orient) {
  if (orient == HORIZONTAL) {
    if (col + size > BOARD_COLS) return false;
    for (uint8_t c = col; c < col + size; c++)
      if (b->grid[row][c] != CELL_EMPTY) return false;
  } else {
    if (row + size > BOARD_ROWS) return false;
    for (uint8_t r = row; r < row + size; r++)
      if (b->grid[r][col] != CELL_EMPTY) return false;
  }
  return true;
}

void board_place_ship(board_t *b, uint8_t col, uint8_t row,
                      uint8_t size, uint8_t type_idx, orientation_t orient) {
  uint8_t idx = b->ships_placed;
  b->ships[idx].col      = col;
  b->ships[idx].row      = row;
  b->ships[idx].size     = size;
  b->ships[idx].type_idx = type_idx;
  b->ships[idx].orient   = orient;
  b->ships[idx].hits     = 0;
  b->ships[idx].sunk     = false;

  if (orient == HORIZONTAL)
    for (uint8_t c = col; c < col + size; c++)
      b->grid[row][c] = CELL_SHIP;
  else
    for (uint8_t r = row; r < row + size; r++)
      b->grid[r][col] = CELL_SHIP;

  b->ships_placed++;
}

bool board_attack(board_t *b, uint8_t col, uint8_t row) {
  if (b->grid[row][col] == CELL_SHIP) {
    b->grid[row][col] = CELL_HIT;
    for (uint8_t i = 0; i < b->ships_placed; i++) {
      ship_t *s = &b->ships[i];
      if (s->sunk) continue;
      bool hit = false;
      if (s->orient == HORIZONTAL)
        hit = (row == s->row && col >= s->col && col < s->col + s->size);
      else
        hit = (col == s->col && row >= s->row && row < s->row + s->size);
      if (hit) {
        s->hits++;
        if (s->hits >= s->size) {
          s->sunk = true;
          b->ships_sunk++;
          if (s->orient == HORIZONTAL)
            for (uint8_t c = s->col; c < s->col + s->size; c++)
              b->grid[s->row][c] = CELL_SUNK;
          else
            for (uint8_t r = s->row; r < s->row + s->size; r++)
              b->grid[r][s->col] = CELL_SUNK;
        }
        break;
      }
    }
    return true;
  } else {
    b->grid[row][col] = CELL_MISS;
    return false;
  }
}

bool board_already_attacked(board_t *b, uint8_t col, uint8_t row) {
  return b->grid[row][col] == CELL_HIT  ||
         b->grid[row][col] == CELL_MISS ||
         b->grid[row][col] == CELL_SUNK;
}

bool board_all_sunk(board_t *b) {
  return b->ships_sunk >= NUM_SHIPS;
}

void board_pixel_to_cell(uint16_t px, uint16_t py, int *col, int *row) {
  *col = ((int)px - BOARD_X) / CELL_SIZE;
  *row = ((int)py - BOARD_Y) / CELL_SIZE;
}

void board_cell_to_pixel(uint8_t col, uint8_t row, uint16_t *px, uint16_t *py) {
  *px = BOARD_X + col * CELL_SIZE;
  *py = BOARD_Y + row * CELL_SIZE;
}

uint8_t board_count_hits(board_t *b) {
  uint8_t n = 0;
  for (uint8_t r = 0; r < BOARD_ROWS; r++)
    for (uint8_t c = 0; c < BOARD_COLS; c++)
      if (b->grid[r][c] == CELL_HIT || b->grid[r][c] == CELL_SUNK)
        n++;
  return n;
}

uint8_t board_count_misses(board_t *b) {
  uint8_t n = 0;
  for (uint8_t r = 0; r < BOARD_ROWS; r++)
    for (uint8_t c = 0; c < BOARD_COLS; c++)
      if (b->grid[r][c] == CELL_MISS)
        n++;
  return n;
}
