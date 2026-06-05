#include "bot.h"
#include <stdlib.h>

static bool hunting = false;
static uint8_t first_hit_c = 0, first_hit_r = 0;
static uint8_t curr_c = 0, curr_r = 0;
static int hunt_dir = 0;
static bool dir_locked = false;

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

void bot_register_result(bool is_hit, bool is_sunk) {
    if (is_sunk) {
        hunting = false;    
        dir_locked = false;
        return;
    }

    if (hunting) {
        if (is_hit) {
            dir_locked = true;
        } else {
            if (dir_locked) {
                hunt_dir = (hunt_dir + 2) % 4;
                curr_c = first_hit_c;
                curr_r = first_hit_r;
            } else {
                hunt_dir = (hunt_dir + 1) % 4;
                curr_c = first_hit_c;
                curr_r = first_hit_r;
            }
        }
    } else if (is_hit) {
        hunting = true; 
        first_hit_c = curr_c;
        first_hit_r = curr_r;
        hunt_dir = 0; 
        dir_locked = false;
    }
}

void bot_choose_attack(board_t *enemy, uint8_t *out_col, uint8_t *out_row) {
    if (!hunting) {
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
        int attempts = 0;

        while (!valid && attempts < 4) {
            int8_t nc = curr_c;
            int8_t nr = curr_r;

            if (hunt_dir == 0) nr--;
            else if (hunt_dir == 1) nc++; 
            else if (hunt_dir == 2) nr++; 
            else if (hunt_dir == 3) nc--; 

            if (nc >= 0 && nc < BOARD_COLS && nr >= 0 && nr < BOARD_ROWS &&
                !board_already_attacked(enemy, (uint8_t)nc, (uint8_t)nr)) {
                curr_c = (uint8_t)nc;
                curr_r = (uint8_t)nr;
                valid = true;
            } else {
                if (dir_locked) {
                    hunt_dir = (hunt_dir + 2) % 4;
                    curr_c = first_hit_c;
                    curr_r = first_hit_r;
                    attempts++;
                } else {
                    hunt_dir = (hunt_dir + 1) % 4;
                    curr_c = first_hit_c;
                    curr_r = first_hit_r;
                    attempts++;
                }
            }
        }

        if (!valid) {
            hunting = false;
            bot_choose_attack(enemy, out_col, out_row);
            return;
        }
    }
    *out_col = curr_c;
    *out_row = curr_r;
}
