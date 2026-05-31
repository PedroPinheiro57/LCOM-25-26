/*
 * game.c — Game logic, two-player serial multiplayer with sprites and animations.
 *
 * RULES:
 *   - Hit → explosion finishes → same player attacks again (no turn switch).
 *   - Miss → explosion finishes → 2 second wait → handover screen → switch player.
 *   - HOST is authoritative for all attack results.
 *   - CLIENT receives attack results and applies them directly (not board_attack).
 *
 * BOARD PERSPECTIVE:
 *   STATE_TURN_P1:
 *     HOST    → draws p2_board, hide_ships=true  (attacking enemy, ships hidden)
 *     CLIENT  → draws p2_board, hide_ships=false (own ships being attacked)
 *   STATE_TURN_P2:
 *     CLIENT  → draws p1_board, hide_ships=true  (attacking enemy, ships hidden)
 *     HOST    → draws p1_board, hide_ships=false (own ships being attacked)
 */
/* Prevent curses.h key constant conflicts with PS/2 scancodes */

#include <lcom/lcf.h>
#include "game.h"
#include "../view/game_menu.h"
#include "../../pedro/lab3/kbc.h"
#include "../view/renderer.h"
#include <string.h>
#include "../view/sprites.h"
#include "../model/keyboard.h"
#include "../model/rtc.h"
#include "../serial/protocol.h"
#include "../view/font.h"
#include "../../pedro/lab5/video.h"

#define TICKS_PER_SEC          30
#define COUNTDOWN_START        5
#define POST_ATTACK_WAIT_TICKS 60   /* 2 seconds at 30 Hz — only after a MISS */
#define RTC_X                  650
#define RTC_Y                  20
 
static game_t  g;
static bool    over  = false;
static bool    dirty = true;
 
static int16_t prev_cx = 0;
static int16_t prev_cy = 0;
 
/* Ticks spent waiting after a miss explosion before switching turn */
static uint8_t post_attack_ticks  = 0;
static bool    waiting_post_attack = false;
 
/* ------------------------------------------------------------------ */
/* Internal helpers                                                   */
/* ------------------------------------------------------------------ */
 
bool game_is_waiting_connect(void) {
    return (g.tag == STATE_WAITING_CONNECT);
}
 
bool game_is_connected(void) {
    return g.connected;
}
 
bool game_is_client_turn(void) {
    return (g.role == ROLE_HOST) && (g.tag == STATE_TURN_P2);
}
 

static void transition(game_state_t next) {
    g.prev = g.tag;
    g.tag  = next;
    dirty  = true;
 
    /* Clear all mouse state on transition to avoid phantom clicks */
    get_mouse_state()->clicked  = false;
    get_mouse_state()->released = false;
    get_mouse_state()->moved    = false;
}
 
/* ------------------------------------------------------------------ */
/* game_init                                                          */
/* ------------------------------------------------------------------ */
void game_init(game_role_t role) {
    memset(&g, 0, sizeof(g));
 
    g.role      = role;
    g.connected = false;
 
    g.remote_cursor_col = -1;
    g.remote_cursor_row = -1;
 
    board_init(&g.p1_board);
    board_init(&g.p2_board);
 
    rtc_read_time(&g.rtc);
 
    font_init();
    cursor_init();
    init_game_sprites();
 
    post_attack_ticks   = 0;
    waiting_post_attack = false;
 
    g.tag  = STATE_WAITING_CONNECT;
    g.prev = STATE_WAITING_CONNECT;
}
 
/* ------------------------------------------------------------------ */
/* game_handle_timer                                                  */
/* ------------------------------------------------------------------ */
void game_handle_timer(void) {
 
    /* ---- 1. RTC update (both roles) ---- */
    g.tick_count++;
    if (g.tick_count >= TICKS_PER_SEC) {
        g.tick_count = 0;
        rtc_read_time(&g.rtc);
    }
 
    /* ---- 2. Countdown timer (HOST only) ---- */
    if (g.role == ROLE_HOST && g.tag == STATE_COUNTDOWN) {
        g.countdown_ticks++;
        if (g.countdown_ticks >= TICKS_PER_SEC) {
            g.countdown_ticks = 0;
            g.countdown_seconds--;
 
            if (g.countdown_seconds <= 0) {
                g.data.turn.player     = 1;
                g.data.turn.cursor_col = 0;
                g.data.turn.cursor_row = 0;
                transition(STATE_TURN_P1);
                proto_send_state(STATE_TURN_P1);
            } else {
                proto_send_countdown((uint8_t)g.countdown_seconds);
            }
        }
    }
 
    /* ---- 3. HOST: after explosion finishes, decide what happens next ----
     *
     * Only HOST drives turn logic. CLIENT just receives state changes.
     *
     * After explosion finishes:
     *   HIT or SUNK → same player attacks again (stay on current turn)
     *   MISS        → start POST_ATTACK_WAIT_TICKS, then handover
     *   Win         → send winner + game over
     */
    if (g.role == ROLE_HOST &&
        (g.tag == STATE_TURN_P1 || g.tag == STATE_TURN_P2) &&
        !waiting_post_attack &&
        renderer_explosion_finished()) {
 
        board_t *enemy = (g.tag == STATE_TURN_P1) ? &g.p2_board : &g.p1_board;
        uint8_t  col   = (uint8_t)renderer_get_expl_col();
        uint8_t  row   = (uint8_t)renderer_get_expl_row();
 
        /* Read the result that board_attack() already wrote */
        uint8_t result;
        if (enemy->grid[row][col] == CELL_MISS) {
            result = ATTACK_MISS;
        } else if (enemy->grid[row][col] == CELL_SUNK) {
            result = ATTACK_SUNK;
        } else {
            result = ATTACK_HIT;
        }
 
        /* Send authoritative result to CLIENT so it can animate */
        proto_send_attack(col, row, result);
 
        /* Check win condition */
        if (board_all_sunk(enemy)) {
            uint8_t winner = (g.tag == STATE_TURN_P1) ? 1 : 2;
            g.data.game_over.winner = winner;
            transition(STATE_GAME_OVER);
            proto_send_winner(winner);
            proto_send_state(STATE_GAME_OVER);
            return;
        }
 
        if (result == ATTACK_HIT || result == ATTACK_SUNK) {
            /* Hit — same player continues, no state change needed */
        } else {
            /* Miss — wait before switching turn */
            waiting_post_attack = true;
            post_attack_ticks   = 0;
        }
    }
 
    /* ---- 4. HOST: post-miss wait then handover ---- */
    if (g.role == ROLE_HOST && waiting_post_attack) {
        post_attack_ticks++;
        if (post_attack_ticks >= POST_ATTACK_WAIT_TICKS) {
            waiting_post_attack = false;
            post_attack_ticks   = 0;
 
            if (g.tag == STATE_TURN_P1) {
                g.data.turn.player = 2;
                g.countdown_ticks  = 0;
                transition(STATE_HANDOVER_P2);
                proto_send_state(STATE_HANDOVER_P2);
            } else {
                g.data.turn.player = 1;
                g.countdown_ticks  = 0;
                transition(STATE_HANDOVER_P1);
                proto_send_state(STATE_HANDOVER_P1);
            }
        }
    }
 
    /* ---- 5. HOST: handover screen — advance to turn after one second ---- */
    if (g.role == ROLE_HOST &&
        (g.tag == STATE_HANDOVER_P1 || g.tag == STATE_HANDOVER_P2)) {
        g.countdown_ticks++;
        if (g.countdown_ticks >= TICKS_PER_SEC) {
            g.countdown_ticks = 0;
            if (g.tag == STATE_HANDOVER_P1) {
                g.data.turn.cursor_col = 0;
                g.data.turn.cursor_row = 0;
                transition(STATE_TURN_P1);
                proto_send_state(STATE_TURN_P1);
            } else {
                g.data.turn.cursor_col = 0;
                g.data.turn.cursor_row = 0;
                transition(STATE_TURN_P2);
                proto_send_state(STATE_TURN_P2);
            }
        }
    }
 
    /* ---- 6. Cursor sync during combat (throttled to 10 Hz) ---- */
    static uint8_t cursor_tick = 0;
    cursor_tick++;
    if (cursor_tick >= 3) {
        cursor_tick = 0;
 
        if (g.role == ROLE_HOST && g.tag == STATE_TURN_P1) {
            proto_send_cursor(
                (uint8_t)g.data.turn.cursor_col,
                (uint8_t)g.data.turn.cursor_row);
        }
 
        if (g.role == ROLE_CLIENT && g.tag == STATE_TURN_P2) {
            proto_send_cursor(
                (uint8_t)g.data.turn.cursor_col,
                (uint8_t)g.data.turn.cursor_row);
        }
    }
}
 
/* ------------------------------------------------------------------ */
/* game_handle_keyboard                                               */
/* ------------------------------------------------------------------ */
void game_handle_keyboard(uint8_t scancode) {
 
    if (g.role == ROLE_CLIENT) {
        if (g.tag == STATE_PLACE_SHIPS_P2 || g.tag == STATE_TURN_P2) {
            /* fall through to local processing below */
        } else {
            if (!g.connected) return;
            proto_send_key(scancode);
            return;
        }
    }
 
    uint8_t code = key_get_code(scancode);
    bool    make = key_is_make(scancode);
 
    switch (g.tag) {
 
        case STATE_WAITING_CONNECT:
            break;
 
        case STATE_MAIN_MENU:
            if (make && code == KEY_UP)
                g.data.menu.selected = (g.data.menu.selected + 2) % 3;
            if (make && code == KEY_DOWN)
                g.data.menu.selected = (g.data.menu.selected + 1) % 3;
            if (!make && code == KEY_ESC)
                over = true;
            if (make && code == KEY_ENTER) {
                switch (g.data.menu.selected) {
                    case 0:
                        board_init(&g.p1_board);
                        board_init(&g.p2_board);
                        g.data.place.player     = 1;
                        g.data.place.ship_idx   = 0;
                        g.data.place.orient     = HORIZONTAL;
                        g.data.place.cursor_col = 0;
                        g.data.place.cursor_row = 0;
                        transition(STATE_PLACE_SHIPS_P1);
                        proto_send_state(STATE_PLACE_SHIPS_P1);
                        break;
                    case 1:
                        transition(STATE_INSTRUCTIONS);
                        proto_send_state(STATE_INSTRUCTIONS);
                        break;
                    case 2:
                        over = true;
                        break;
                }
            }
            break;
 
        case STATE_INSTRUCTIONS:
            if (make && (code == KEY_ESC || code == KEY_ENTER)) {
                transition(STATE_MAIN_MENU);
                proto_send_state(STATE_MAIN_MENU);
            }
            break;
 
        case STATE_PLACE_SHIPS_P1:
        case STATE_PLACE_SHIPS_P2: {
            if (make && code == KEY_UP)
                g.data.place.cursor_row =
                    (g.data.place.cursor_row > 0)
                    ? g.data.place.cursor_row - 1 : 0;
            if (make && code == KEY_DOWN)
                g.data.place.cursor_row =
                    (g.data.place.cursor_row < BOARD_ROWS - 1)
                    ? g.data.place.cursor_row + 1 : BOARD_ROWS - 1;
            if (make && code == KEY_LEFT)
                g.data.place.cursor_col =
                    (g.data.place.cursor_col > 0)
                    ? g.data.place.cursor_col - 1 : 0;
            if (make && code == KEY_RIGHT)
                g.data.place.cursor_col =
                    (g.data.place.cursor_col < BOARD_COLS - 1)
                    ? g.data.place.cursor_col + 1 : BOARD_COLS - 1;
 
            if (make && code == KEY_R)
                g.data.place.orient ^= 1;
 
            if (make && code == KEY_ENTER) {
                board_t *b   = (g.tag == STATE_PLACE_SHIPS_P1)
                               ? &g.p1_board : &g.p2_board;
                uint8_t col  = (uint8_t)g.data.place.cursor_col;
                uint8_t row  = (uint8_t)g.data.place.cursor_row;
                uint8_t size = SHIP_SIZES[g.data.place.ship_idx];
                orientation_t orient = (orientation_t)g.data.place.orient;
 
                if (board_can_place(b, col, row, size, orient)) {
                    board_place_ship(b, col, row, size, orient);
 
                    if (g.role == ROLE_CLIENT) {
                        proto_send_ship_place(col, row, size, (uint8_t)orient);
                    }
 
                    g.data.place.ship_idx++;
 
                    if (g.data.place.ship_idx >= NUM_SHIPS) {
                        if (g.tag == STATE_PLACE_SHIPS_P1) {
                            g.data.place.ship_idx   = 0;
                            g.data.place.orient     = HORIZONTAL;
                            g.data.place.cursor_col = 0;
                            g.data.place.cursor_row = 0;
                            transition(STATE_PLACE_SHIPS_WAITING);
                            proto_send_state(STATE_PLACE_SHIPS_P2);
                        } else {
                            transition(STATE_PLACE_SHIPS_WAITING);
                            proto_send_done_placing();
                        }
                    }
                }
            }
 
            if (!make && code == KEY_ESC)
                transition(STATE_PAUSED);
            break;
        }
 
        case STATE_PLACE_SHIPS_WAITING:
            break;
 
        case STATE_COUNTDOWN:
            break;
 
        case STATE_TURN_P1:
        case STATE_TURN_P2: {
            /* STRICT GATING: only the correct player can act */
            if (g.tag == STATE_TURN_P1 && g.role != ROLE_HOST)   break;
            if (g.tag == STATE_TURN_P2 && g.role != ROLE_CLIENT) break;
 
            /* Block input while explosion is playing */
            if (renderer_is_exploding()) break;
 
            if (make && code == KEY_UP)
                g.data.turn.cursor_row =
                    (g.data.turn.cursor_row > 0)
                    ? g.data.turn.cursor_row - 1 : 0;
            if (make && code == KEY_DOWN)
                g.data.turn.cursor_row =
                    (g.data.turn.cursor_row < BOARD_ROWS - 1)
                    ? g.data.turn.cursor_row + 1 : BOARD_ROWS - 1;
            if (make && code == KEY_LEFT)
                g.data.turn.cursor_col =
                    (g.data.turn.cursor_col > 0)
                    ? g.data.turn.cursor_col - 1 : 0;
            if (make && code == KEY_RIGHT)
                g.data.turn.cursor_col =
                    (g.data.turn.cursor_col < BOARD_COLS - 1)
                    ? g.data.turn.cursor_col + 1 : BOARD_COLS - 1;
 
            if (make && (code == KEY_ENTER || code == KEY_SPACE)) {
                board_t *enemy = (g.tag == STATE_TURN_P1)
                                 ? &g.p2_board : &g.p1_board;
                uint8_t col = (uint8_t)g.data.turn.cursor_col;
                uint8_t row = (uint8_t)g.data.turn.cursor_row;
 
                if (!board_already_attacked(enemy, col, row)) {
                    if (g.role == ROLE_HOST) {
                        board_attack(enemy, col, row);
                        bool is_hit = (enemy->grid[row][col] == CELL_HIT ||
                                       enemy->grid[row][col] == CELL_SUNK);
                        start_explosion(col, row, is_hit);
                    } else {
                        /* CLIENT: send to HOST, wait for echo */
                        proto_send_attack(col, row, 0);
                    }
                }
            }
 
            if (!make && code == KEY_ESC)
                transition(STATE_PAUSED);
            break;
        }
 
        case STATE_HANDOVER_P1:
        case STATE_HANDOVER_P2:
            break;
 
        case STATE_PAUSED:
            if (make && (code == KEY_UP || code == KEY_DOWN))
                g.data.pause.selected = (g.data.pause.selected + 1) % 2;
            if (make && code == KEY_ENTER) {
                if (g.data.pause.selected == 0) transition(g.prev);
                if (g.data.pause.selected == 1) over = true;
            }
            if (!make && code == KEY_ESC)
                transition(g.prev);
            break;
 
        case STATE_GAME_OVER:
            if (make && code == KEY_ENTER) {
                board_init(&g.p1_board);
                board_init(&g.p2_board);
                g.data.menu.selected = 0;
                transition(STATE_MAIN_MENU);
                proto_send_state(STATE_MAIN_MENU);
            }
            if (!make && code == KEY_ESC)
                over = true;
            break;
 
        default:
            break;
    }
}
 
/* ------------------------------------------------------------------ */
/* game_handle_mouse                                                  */
/* ------------------------------------------------------------------ */
void game_handle_mouse(mouse_state_t *ms) {
 
    if (g.role == ROLE_CLIENT) {
        if (g.tag == STATE_PLACE_SHIPS_P2 || g.tag == STATE_TURN_P2) {
            /* fall through to local processing below */
        } else {
            if (!g.connected) return;
            if (ms->moved || ms->clicked) {
                proto_send_mouse(get_mouse_buf());
            }
            return;
        }
    }
 
    int col, row;
 
    switch (g.tag) {
 
        case STATE_MAIN_MENU: {
            int hover = menu_mouse_hover(ms->x, ms->y);
            cursor_set_mode(hover >= 0 ? CURSOR_HOVER : CURSOR_NORMAL);
            if (ms->moved && hover >= 0 && hover != g.data.menu.selected)
                g.data.menu.selected = hover;
            if (ms->clicked && hover >= 0) {
                switch (hover) {
                    case 0:
                        board_init(&g.p1_board);
                        board_init(&g.p2_board);
                        g.data.place.player     = 1;
                        g.data.place.ship_idx   = 0;
                        g.data.place.orient     = HORIZONTAL;
                        g.data.place.cursor_col = 0;
                        g.data.place.cursor_row = 0;
                        transition(STATE_PLACE_SHIPS_P1);
                        proto_send_state(STATE_PLACE_SHIPS_P1);
                        break;
                    case 1:
                        transition(STATE_INSTRUCTIONS);
                        proto_send_state(STATE_INSTRUCTIONS);
                        break;
                    case 2:
                        over = true;
                        break;
                }
            }
            break;
        }
 
        case STATE_PLACE_SHIPS_P1:
        case STATE_PLACE_SHIPS_P2: {
            if (g.tag == STATE_PLACE_SHIPS_P1 && g.role != ROLE_HOST)   break;
            if (g.tag == STATE_PLACE_SHIPS_P2 && g.role != ROLE_CLIENT) break;
 
            board_t *b = (g.tag == STATE_PLACE_SHIPS_P1)
                         ? &g.p1_board : &g.p2_board;
 
            if (ms->moved) {
                board_pixel_to_cell(ms->x, ms->y, &col, &row);
                if (col >= 0 && col < BOARD_COLS &&
                    row >= 0 && row < BOARD_ROWS) {
                    g.data.place.cursor_col = col;
                    g.data.place.cursor_row = row;
                }
            }
 
            if (ms->clicked) {
                col = g.data.place.cursor_col;
                row = g.data.place.cursor_row;
 
                uint8_t size         = SHIP_SIZES[g.data.place.ship_idx];
                orientation_t orient = (orientation_t)g.data.place.orient;
 
                if (board_can_place(b, col, row, size, orient)) {
                    board_place_ship(b, col, row, size, orient);
 
                    if (g.role == ROLE_CLIENT) {
                        proto_send_ship_place((uint8_t)col, (uint8_t)row,
                                              size, (uint8_t)orient);
                    }
 
                    g.data.place.ship_idx++;
                    if (g.data.place.ship_idx >= NUM_SHIPS) {
                        if (g.tag == STATE_PLACE_SHIPS_P1) {
                            g.data.place.ship_idx   = 0;
                            g.data.place.orient     = HORIZONTAL;
                            g.data.place.cursor_col = 0;
                            g.data.place.cursor_row = 0;
                            transition(STATE_PLACE_SHIPS_WAITING);
                            proto_send_state(STATE_PLACE_SHIPS_P2);
                        } else {
                            transition(STATE_PLACE_SHIPS_WAITING);
                            proto_send_done_placing();
                        }
                    }
                }
            }
            break;
        }
 
        case STATE_TURN_P1: {
            if (g.role != ROLE_HOST) break;
            if (renderer_is_exploding()) break;
 
            board_t *enemy = &g.p2_board;
 
            if (ms->moved) {
                board_pixel_to_cell(ms->x, ms->y, &col, &row);
                if (col >= 0 && col < BOARD_COLS &&
                    row >= 0 && row < BOARD_ROWS) {
                    g.data.turn.cursor_col = col;
                    g.data.turn.cursor_row = row;
                }
            }
 
            if (ms->clicked) {
                col = g.data.turn.cursor_col;
                row = g.data.turn.cursor_row;
 
                if (!board_already_attacked(enemy, col, row)) {
                    board_attack(enemy, col, row);
                    bool is_hit = (enemy->grid[row][col] == CELL_HIT ||
                                   enemy->grid[row][col] == CELL_SUNK);
                    start_explosion(col, row, is_hit);
                }
            }
            break;
        }
 
        case STATE_TURN_P2: {
            if (g.role != ROLE_CLIENT) break;
            if (renderer_is_exploding()) break;
 
            board_t *enemy = &g.p1_board;
 
            if (ms->moved) {
                board_pixel_to_cell(ms->x, ms->y, &col, &row);
                if (col >= 0 && col < BOARD_COLS &&
                    row >= 0 && row < BOARD_ROWS) {
                    g.data.turn.cursor_col = col;
                    g.data.turn.cursor_row = row;
                }
            }
 
            if (ms->clicked) {
                col = g.data.turn.cursor_col;
                row = g.data.turn.cursor_row;
 
                if (!board_already_attacked(enemy, col, row)) {
                    /* Send to HOST, wait for authoritative echo */
                    proto_send_attack((uint8_t)col, (uint8_t)row, 0);
                }
            }
            break;
        }
 
        default:
            break;
    }
}
 
/* ------------------------------------------------------------------ */
/* game_handle_serial_msg                                             */
/* ------------------------------------------------------------------ */
void game_handle_serial_msg(const serial_msg_t *msg) {
 
    /* ============================================================== */
    /* HOST                                                           */
    /* ============================================================== */
    if (g.role == ROLE_HOST) {
        switch (msg->type) {
 
            case MSG_HELLO:
                if (!g.connected) {
                    g.connected = true;
                    proto_send_hello_ack();
                    g.data.menu.selected = 0;
                    transition(STATE_MAIN_MENU);
                    proto_send_state(STATE_MAIN_MENU);
                }
                break;
 
            case MSG_DONE_PLACING:
                if (g.tag == STATE_PLACE_SHIPS_WAITING) {
                    g.countdown_seconds = COUNTDOWN_START;
                    g.countdown_ticks   = 0;
                    transition(STATE_COUNTDOWN);
                    proto_send_state(STATE_COUNTDOWN);
                    proto_send_countdown(g.countdown_seconds);
                }
                break;
 
            case MSG_SHIP_PLACE: {
                uint8_t col  = msg->payload.ship.col;
                uint8_t row  = msg->payload.ship.row;
                uint8_t so   = msg->payload.ship.size_orient;
                uint8_t size = (so >> 4) & 0x0F;
                uint8_t ori  = so & 0x01;
                board_place_ship(&g.p2_board, col, row, size, (orientation_t)ori);
                break;
            }
 
            case MSG_KEY:
                if (g.tag == STATE_TURN_P2) {
                    game_role_t saved = g.role;
                    g.role = ROLE_HOST;
                    game_handle_keyboard(msg->payload.key.scancode);
                    g.role = saved;
                }
                break;
 
            case MSG_ATTACK: {
                /*
                 * CLIENT (P2) sent an attack request.
                 * HOST applies it authoritatively to p1_board and plays
                 * the explosion. Result is sent to CLIENT only after the
                 * explosion finishes (game_handle_timer step 3).
                 */
                if (g.tag != STATE_TURN_P2) break;
                if (renderer_is_exploding()) break;
 
                uint8_t col    = msg->payload.attack.col;
                uint8_t row    = msg->payload.attack.row;
                board_t *enemy = &g.p1_board;
 
                if (board_already_attacked(enemy, col, row)) break;
 
                board_attack(enemy, col, row);
                bool is_hit = (enemy->grid[row][col] == CELL_HIT ||
                               enemy->grid[row][col] == CELL_SUNK);
                start_explosion(col, row, is_hit);
                break;
            }
 
            case MSG_MOUSE:
                break;
 
            case MSG_CURSOR:
                g.remote_cursor_col = (int8_t)msg->payload.cursor.col;
                g.remote_cursor_row = (int8_t)msg->payload.cursor.row;
                break;
 
            default:
                break;
        }
        return;
    }
 
    /* ============================================================== */
    /* CLIENT                                                         */
    /* ============================================================== */
    switch (msg->type) {
 
        case MSG_HELLO_ACK:
            g.connected = true;
            break;
 
        case MSG_STATE:
            transition((game_state_t)msg->payload.state.state);
            if (g.tag == STATE_TURN_P1 || g.tag == STATE_TURN_P2) {
                g.data.turn.cursor_col = 0;
                g.data.turn.cursor_row = 0;
                g.remote_cursor_col    = -1;
                g.remote_cursor_row    = -1;
            }
            if (g.tag == STATE_PLACE_SHIPS_P2) {
                g.data.place.ship_idx   = 0;
                g.data.place.orient     = HORIZONTAL;
                g.data.place.cursor_col = 0;
                g.data.place.cursor_row = 0;
            }
            break;
 
        case MSG_ATTACK: {
            /*
             * HOST sent the authoritative attack result.
             * CLIENT applies it to the correct board and plays the
             * explosion — identical visual behaviour to HOST.
             *
             * STATE_TURN_P1: HOST attacked → target is CLIENT's own
             *                ships (p2_board from CLIENT's perspective).
             * STATE_TURN_P2: HOST echoed back CLIENT's attack → target
             *                is HOST's ships (p1_board from CLIENT's
             *                perspective).
             */
            uint8_t col    = msg->payload.attack.col;
            uint8_t row    = msg->payload.attack.row;
            uint8_t result = msg->payload.attack.result;
 
            board_t *target = (g.tag == STATE_TURN_P1) ? &g.p2_board
                                                        : &g.p1_board;
 
            if (!board_already_attacked(target, col, row)) {
                if (result == ATTACK_MISS) {
                    target->grid[row][col] = CELL_MISS;
                } else if (result == ATTACK_HIT) {
                    target->grid[row][col] = CELL_HIT;
                } else if (result == ATTACK_SUNK) {
                    /*
                     * Mark all cells of the sunk ship CELL_SUNK so the
                     * ship sprite appears correctly on CLIENT's screen.
                     * HOST already did this via board_attack() on its
                     * own copy; CLIENT must replicate it here.
                     */
                    target->grid[row][col] = CELL_SUNK;
                    target->ships_sunk++;
 
                    for (uint8_t i = 0; i < target->ships_placed; i++) {
                        ship_t *s = &target->ships[i];
                        if (s->sunk) continue;
                        bool contains = false;
                        if (s->orient == HORIZONTAL)
                            contains = (row == s->row &&
                                        col >= s->col &&
                                        col < s->col + s->size);
                        else
                            contains = (col == s->col &&
                                        row >= s->row &&
                                        row < s->row + s->size);
                        if (contains) {
                            s->sunk = true;
                            s->hits = s->size;
                            if (s->orient == HORIZONTAL)
                                for (uint8_t c = s->col;
                                     c < s->col + s->size; c++)
                                    target->grid[s->row][c] = CELL_SUNK;
                            else
                                for (uint8_t r = s->row;
                                     r < s->row + s->size; r++)
                                    target->grid[r][s->col] = CELL_SUNK;
                            break;
                        }
                    }
                }
 
                bool is_hit = (result == ATTACK_HIT || result == ATTACK_SUNK);
                start_explosion(col, row, is_hit);
            }
            break;
        }
 
        case MSG_CURSOR:
            g.remote_cursor_col = (int8_t)msg->payload.cursor.col;
            g.remote_cursor_row = (int8_t)msg->payload.cursor.row;
            break;
 
        case MSG_COUNTDOWN:
            g.countdown_seconds = msg->payload.countdown.seconds;
            break;
 
        case MSG_WINNER:
            g.data.game_over.winner = msg->payload.winner.winner;
            break;
 
        default:
            break;
    }
}

 
/* ------------------------------------------------------------------ */
/* Cursor helpers                                                     */
/* ------------------------------------------------------------------ */
void game_erase_cursor(void) {
    vg_draw_rectangle_project(prev_cx, prev_cy, 11, 11, 0x000000);
}
 
void game_save_cursor(int16_t x, int16_t y) {
    prev_cx = x;
    prev_cy = y;
}
 
bool game_is_over(void) { return over; }
const game_t *game_get_state(void) { return &g; }

