#pragma once
#include <stdint.h>
#include <stdbool.h>

#define BOARD_COLS   10
#define BOARD_ROWS   10
#define CELL_SIZE    45
#define BOARD_X      175
#define BOARD_Y      90
#define NUM_SHIPS    5

typedef enum {
  CELL_EMPTY,
  CELL_SHIP,
  CELL_HIT,
  CELL_MISS,
  CELL_SUNK
} cell_state_t;

typedef enum {
  HORIZONTAL,
  VERTICAL
} orientation_t;

typedef struct {
  uint8_t       col, row;
  uint8_t       size;
  uint8_t       type_idx;   /* index into SHIP_SIZES/sprite arrays */
  orientation_t orient;
  uint8_t       hits;
  bool          sunk;
} ship_t;

typedef struct {
  cell_state_t grid[BOARD_ROWS][BOARD_COLS];
  ship_t       ships[NUM_SHIPS];
  uint8_t      ships_placed;
  uint8_t      ships_sunk;
} board_t;

static const uint8_t SHIP_SIZES[NUM_SHIPS] = {5, 4, 3, 3, 2};
static const char   *SHIP_NAMES[NUM_SHIPS] = {
  "CARRIER", "BATTLESHIP", "CRUISER", "SUBMARINE", "DESTROYER"
};

void    board_init(board_t *b);
bool    board_can_place(board_t *b, uint8_t col, uint8_t row, uint8_t size, orientation_t orient);
void    board_place_ship(board_t *b, uint8_t col, uint8_t row, uint8_t size, uint8_t type_idx, orientation_t orient);
bool    board_attack(board_t *b, uint8_t col, uint8_t row);
bool    board_already_attacked(board_t *b, uint8_t col, uint8_t row);
bool    board_all_sunk(board_t *b);
void    board_pixel_to_cell(uint16_t px, uint16_t py, int *col, int *row);
void    board_cell_to_pixel(uint8_t col, uint8_t row, uint16_t *px, uint16_t *py);
uint8_t board_count_hits(board_t *b);
uint8_t board_count_misses(board_t *b);
