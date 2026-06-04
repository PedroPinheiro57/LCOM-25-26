/**
 * @file board.h
 * @brief Battleship game board: data model and grid logic.
 *
 * Defines the 10×10 grid, ship placement, attack resolution, and
 * pixel↔cell coordinate conversion used by both the renderer and the
 * game controller.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

/** @brief Number of columns on the grid. */
#define BOARD_COLS  10
/** @brief Number of rows on the grid. */
#define BOARD_ROWS  10
/** @brief Side length of a single cell in pixels. */
#define CELL_SIZE   45
/** @brief X pixel coordinate of the grid's top-left corner. */
#define BOARD_X     175
/** @brief Y pixel coordinate of the grid's top-left corner. */
#define BOARD_Y     90
/** @brief Total number of ships per player. */
#define NUM_SHIPS   5

/**
 * @brief Possible states for a single grid cell.
 */
typedef enum {
    CELL_EMPTY, /**< Untouched water. */
    CELL_SHIP,  /**< Part of a ship that has not been hit yet. */
    CELL_HIT,   /**< A ship cell that was hit. */
    CELL_MISS,  /**< An attack that landed on empty water. */
    CELL_SUNK   /**< Part of a fully-sunk ship. */
} cell_state_t;

/**
 * @brief Ship orientation on the grid.
 */
typedef enum {
    HORIZONTAL, /**< Ship spans columns left-to-right. */
    VERTICAL    /**< Ship spans rows top-to-bottom.    */
} orientation_t;

/**
 * @brief All data associated with a single ship.
 */
typedef struct {
    uint8_t       col;       /**< Grid column of the ship's top-left cell. */
    uint8_t       row;       /**< Grid row of the ship's top-left cell.    */
    uint8_t       size;      /**< Number of cells this ship occupies.      */
    uint8_t       type_idx;  /**< Index into @ref SHIP_SIZES / sprite arrays. */
    orientation_t orient;    /**< Horizontal or vertical placement.        */
    uint8_t       hits;      /**< Number of cells hit so far.              */
    bool          sunk;      /**< @c true once all cells have been hit.    */
} ship_t;

/**
 * @brief The full board state for one player.
 *
 * Tracks the cell grid and the list of ships.  Both players own one of
 * these; the host keeps both.
 */
typedef struct {
    cell_state_t grid[BOARD_ROWS][BOARD_COLS]; /**< Cell state for every grid position. */
    ship_t       ships[NUM_SHIPS];             /**< Per-ship data.                      */
    uint8_t      ships_placed;                 /**< How many ships have been placed so far. */
    uint8_t      ships_sunk;                   /**< How many of this player's ships are sunk. */
} board_t;

/** @brief Canonical ship lengths indexed by type, largest to smallest. */
static const uint8_t SHIP_SIZES[NUM_SHIPS] = {5, 4, 3, 3, 2};
/** @brief Display names for each ship type (same index order as @ref SHIP_SIZES). */
static const char   *SHIP_NAMES[NUM_SHIPS] = {
    "CARRIER", "BATTLESHIP", "CRUISER", "SUBMARINE", "DESTROYER"
};

/**
 * @brief Clears the board to an all-empty state.
 * @param b Board to initialise.
 */
void board_init(board_t *b);

/**
 * @brief Checks whether a ship can legally be placed at the given position.
 *
 * Returns @c false if any cell the ship would occupy is already taken
 * or if the ship would extend beyond the grid boundaries.
 *
 * @param b      Board to check against.
 * @param col    Column of the top-left cell.
 * @param row    Row of the top-left cell.
 * @param size   Number of cells the ship occupies.
 * @param orient Placement direction.
 * @return @c true if the placement is valid.
 */
bool board_can_place(board_t *b, uint8_t col, uint8_t row,
                     uint8_t size, orientation_t orient);

/**
 * @brief Commits a ship to the board, marking its cells as @ref CELL_SHIP.
 *
 * Does not validate the placement — call @ref board_can_place first.
 *
 * @param b         Target board.
 * @param col       Column of the top-left cell.
 * @param row       Row of the top-left cell.
 * @param size      Ship length in cells.
 * @param type_idx  Ship type index (used to pick the correct sprite).
 * @param orient    Placement orientation.
 */
void board_place_ship(board_t *b, uint8_t col, uint8_t row,
                      uint8_t size, uint8_t type_idx, orientation_t orient);

/**
 * @brief Resolves an attack on the given cell.
 *
 * Updates the cell to @ref CELL_HIT or @ref CELL_MISS, increments the
 * ship's hit counter, and marks it as sunk when all cells are hit.
 *
 * @param b   Board being attacked.
 * @param col Target column.
 * @param row Target row.
 * @return @c true if the attack hit a ship.
 */
bool board_attack(board_t *b, uint8_t col, uint8_t row);

/**
 * @brief Returns @c true if the cell has already been attacked (hit or miss).
 * @param b   Board to query.
 * @param col Target column.
 * @param row Target row.
 */
bool board_already_attacked(board_t *b, uint8_t col, uint8_t row);

/**
 * @brief Returns @c true when every ship on the board has been sunk.
 * @param b Board to inspect.
 */
bool board_all_sunk(board_t *b);

/**
 * @brief Converts a pixel coordinate to the corresponding grid cell.
 *
 * The output values may be negative or >= @ref BOARD_COLS / @ref BOARD_ROWS
 * if the pixel is outside the board area — callers should validate.
 *
 * @param px   Pixel X coordinate.
 * @param py   Pixel Y coordinate.
 * @param col  Output column index.
 * @param row  Output row index.
 */
void board_pixel_to_cell(uint16_t px, uint16_t py, int *col, int *row);

/**
 * @brief Converts a grid cell to the pixel coordinate of its top-left corner.
 *
 * @param col  Grid column.
 * @param row  Grid row.
 * @param px   Output pixel X.
 * @param py   Output pixel Y.
 */
void board_cell_to_pixel(uint8_t col, uint8_t row, uint16_t *px, uint16_t *py);

/**
 * @brief Counts the total number of @ref CELL_HIT cells on the board.
 * @param b Board to inspect.
 * @return Number of hits.
 */
uint8_t board_count_hits(board_t *b);

/**
 * @brief Counts the total number of @ref CELL_MISS cells on the board.
 * @param b Board to inspect.
 * @return Number of misses.
 */
uint8_t board_count_misses(board_t *b);
