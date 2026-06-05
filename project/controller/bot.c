#include "bot.h"
#include <stdlib.h>

typedef enum {
    MODE_RANDOM,
    MODE_SEARCH_DIR,
    MODE_DESTROY
} bot_mode_t;

static bot_mode_t mode = MODE_RANDOM;
static uint8_t first_hit_c = 0, first_hit_r = 0;
static uint8_t curr_c = 0, curr_r = 0;
static int hunt_dir = 0; 
static int search_dirs_tried = 0;

void bot_place_ships(board_t *b) {
    for (int i = 0; i < NUM_SHIPS; i++) {
        uint8_t size = SHIP_SIZES[i];
        bool placed = false;
        while (!placed) {
            uint8_t col = rand() % BOARD_COLS;
            uint8_t row = rand() % BOARD_ROWS;
            orientation_t ori = (rand() % 2 == 0) ? HORIZONTAL : VERTICAL;
            if (board_can_place(b, col, row, size, ori)) {
                board_place_ship(b, col, row, size, (uint8_t)i, ori);
                placed = true;
            }
        }
    }
}

void bot_reset(void) {
    mode = MODE_RANDOM;
    search_dirs_tried = 0;
    hunt_dir = 0;
    first_hit_c = 0;
    first_hit_r = 0;
    curr_c = 0;
    curr_r = 0;
}

void bot_register_result(bool is_hit, bool is_sunk) {
    if (is_sunk) {
        mode = MODE_RANDOM;
        return;
    }

    if (mode == MODE_RANDOM) {
        if (is_hit) {
            mode = MODE_SEARCH_DIR;
            first_hit_c = curr_c;
            first_hit_r = curr_r;
            hunt_dir = 0; 
            search_dirs_tried = 0;
        }
    } else if (mode == MODE_SEARCH_DIR) {
        if (is_hit) {
            mode = MODE_DESTROY;
        } else {
            hunt_dir = (hunt_dir + 1) % 4;
            search_dirs_tried++;
            curr_c = first_hit_c;
            curr_r = first_hit_r;
            
            if (search_dirs_tried >= 4) {
                mode = MODE_RANDOM;
            }
        }
    } else if (mode == MODE_DESTROY) {
        if (!is_hit) {
            hunt_dir = (hunt_dir + 2) % 4;
            curr_c = first_hit_c;
            curr_r = first_hit_r;
        }
    }
}

void bot_choose_attack(board_t *enemy, uint8_t *out_col, uint8_t *out_row) {
    if (mode == MODE_RANDOM) {
        bool valid = false;
        while (!valid) {
            curr_c = rand() % BOARD_COLS;
            curr_r = rand() % BOARD_ROWS;
            if (!board_already_attacked(enemy, curr_c, curr_r)) {
                valid = true;
            }
        }
    } else {
        bool valid = false;
        int fallback_attempts = 0;

        while (!valid && fallback_attempts < 4) {
            int8_t nc = curr_c;
            int8_t nr = curr_r;

            if (hunt_dir == 0) nr--;
            else if (hunt_dir == 1) nc++; 
            else if (hunt_dir == 2) nr++; 
            else if (hunt_dir == 3) nc--; 

            bool cell_blocked = false;

            if (nc >= 0 && nc < BOARD_COLS && nr >= 0 && nr < BOARD_ROWS) {
                if (!board_already_attacked(enemy, (uint8_t)nc, (uint8_t)nr)) {
                    curr_c = (uint8_t)nc;
                    curr_r = (uint8_t)nr;
                    valid = true;
                } else if (enemy->grid[nr][nc] == CELL_HIT) {
                    curr_c = (uint8_t)nc;
                    curr_r = (uint8_t)nr;
                    continue; 
                } else {
                    cell_blocked = true;
                }
            } else {
                cell_blocked = true; 
            }

            if (cell_blocked) {
                if (mode == MODE_SEARCH_DIR) {
                    hunt_dir = (hunt_dir + 1) % 4;
                    search_dirs_tried++;
                    curr_c = first_hit_c;
                    curr_r = first_hit_r;
                    if (search_dirs_tried >= 4) {
                        mode = MODE_RANDOM;
                        bot_choose_attack(enemy, out_col, out_row);
                        return;
                    }
                } else if (mode == MODE_DESTROY) {
                    hunt_dir = (hunt_dir + 2) % 4;
                    curr_c = first_hit_c;
                    curr_r = first_hit_r;
                    fallback_attempts++;
                }
            }
        }

        if (!valid) {
            mode = MODE_RANDOM;
            bot_choose_attack(enemy, out_col, out_row);
            return;
        }
    }
    
    *out_col = curr_c;
    *out_row = curr_r;
}
