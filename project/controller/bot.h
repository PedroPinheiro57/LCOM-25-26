#pragma once
#include <stdint.h>
#include "../model/board.h"

/**
 * @file bot.h
 * @brief AI logic for the Single Player Bot.
 *
 * Contains the state machine and functions to make the bot place ships 
 * and play the game using a "Hunt and Target" algorithm.
 */

/**
 * @brief Places all 5 ships randomly on the board for the bot.
 * @param b Pointer to the bot's board.
 */
void bot_place_ships(board_t *b);

/**
 * @brief Chooses a random valid target for the bot to attack.
 * @param enemy   Pointer to the player's board.
 * @param out_col Pointer to store the chosen column.
 * @param out_row Pointer to store the chosen row.
 */
void bot_choose_attack(board_t *enemy, uint8_t *out_col, uint8_t *out_row);

/**
 * @brief Informs the bot about the result of its last attack so it can plan the next one.
 * @param is_hit  true if the last attack hit an enemy ship cell; false if it hit water (miss).
 * @param is_sunk true if the last attack resulted in completely destroying a ship; false otherwise.
 */
void bot_register_result(bool is_hit, bool is_sunk);

/**
 * @brief Resets the bot's memory (hunting state, coordinates) for a new game.
 */
void bot_reset(void);
