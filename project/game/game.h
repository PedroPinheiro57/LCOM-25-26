#pragma once
/*
 * game.h — Game state definitions, two-player serial multiplayer.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../devices/mouse.h"
#include "../devices/rtc.h"
#include "../serial/protocol.h"

/* ------------------------------------------------------------------ */
/* Role                                                               */
/* ------------------------------------------------------------------ */
typedef enum {
    ROLE_HOST,    /* original VM: runs logic, sends state to client   */
    ROLE_CLIENT   /* cloned  VM: forwards input, renders from host    */
} game_role_t;

/* ------------------------------------------------------------------ */
/* Game states                                                        */
/* ------------------------------------------------------------------ */
typedef enum {
    STATE_MAIN_MENU,
    STATE_INSTRUCTIONS,
    STATE_WAITING_CONNECT,
    STATE_PLACE_SHIPS_P1,
    STATE_HANDOVER_P2,
    STATE_PLACE_SHIPS_P2,
    STATE_HANDOVER_P1,
    STATE_PLACE_SHIPS_WAITING,  /* waiting for opponent to finish placing */
    STATE_COUNTDOWN,
    STATE_TURN_P1,
    STATE_TURN_P2,
    STATE_PAUSED,
    STATE_GAME_OVER,
    STATE_EXIT
} game_state_t;

#include "board.h"

/* ------------------------------------------------------------------ */
/* Main game structure                                                */
/* ------------------------------------------------------------------ */
typedef struct {
    game_state_t tag;
    game_state_t prev;

    game_role_t  role;
    bool         connected;

    board_t p1_board;
    board_t p2_board;

    rtc_time_t rtc;
    uint32_t   tick_count;

    uint32_t countdown_ticks;
    uint8_t  countdown_seconds;

    int8_t remote_cursor_col;
    int8_t remote_cursor_row;

    union {
        struct { int selected; } menu;
        struct { int selected; } pause;

        struct {
            int player;
            int ship_idx;
            int orient;
            int cursor_col;
            int cursor_row;
        } place;

        struct {
            int  player;
            int  cursor_col;
            int  cursor_row;
            bool last_hit;
        } turn;

        struct { int winner; } game_over;

    } data;

} game_t;

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */
void game_init(game_role_t role);
void game_handle_timer(void);
void game_handle_keyboard(uint8_t scancode);
void game_handle_mouse(mouse_state_t *ms);
void game_draw(void);
void game_erase_cursor(void);
void game_save_cursor(int16_t x, int16_t y);
bool game_is_over(void);
void game_handle_serial_msg(const serial_msg_t *msg);

/* State query helpers (used by main.c) */
bool game_is_waiting_connect(void);
bool game_is_connected(void);
bool game_is_client_turn(void);
