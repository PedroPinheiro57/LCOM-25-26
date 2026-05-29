/*
 * game.c — Game logic, two-player serial multiplayer.
 */

#include "game.h"
#include "menu.h"
#include "renderer.h"
#include <lcom/lcf.h>
#include <string.h>
#include "../video/font.h"
#include "../video/sprites.h"
#include "../devices/keyboard.h"
#include "../devices/rtc.h"
#include "../serial/protocol.h"
#include "../../pedro/lab5/video.h"
#include "../../pedro/lab3/kbc.h"

#define TICKS_PER_SEC    30
#define COUNTDOWN_START  5
#define RTC_X  650
#define RTC_Y   20

static game_t  g;
static bool    over  = false;
static bool    dirty = true;

static int16_t prev_cx = 0;
static int16_t prev_cy = 0;

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

static void rtc_format(const rtc_time_t *t, char buf[9]) {
    buf[0] = '0' + t->hours   / 10;
    buf[1] = '0' + t->hours   % 10;
    buf[2] = ':';
    buf[3] = '0' + t->minutes / 10;
    buf[4] = '0' + t->minutes % 10;
    buf[5] = ':';
    buf[6] = '0' + t->seconds / 10;
    buf[7] = '0' + t->seconds % 10;
    buf[8] = '\0';
}

static void draw_rtc(void) {
    char buf[9];
    rtc_format(&g.rtc, buf);
    draw_string(buf, RTC_X, RTC_Y, 0xAAAAAA, 2);
}

static void transition(game_state_t next) {
    g.prev = g.tag;
    g.tag  = next;
    dirty  = true;
    /* Reset mouse flags on every state change to avoid phantom clicks */
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

    g.tag  = STATE_WAITING_CONNECT;
    g.prev = STATE_WAITING_CONNECT;
}

/* ------------------------------------------------------------------ */
/* game_handle_timer                                                  */
/* ------------------------------------------------------------------ */
void game_handle_timer(void) {

    /* 1. RTC update (both roles) */
    g.tick_count++;
    if (g.tick_count >= TICKS_PER_SEC) {
        g.tick_count = 0;
        rtc_read_time(&g.rtc);
    }

    /* 2. Countdown timer (HOST only) */
    if (g.role == ROLE_HOST && g.tag == STATE_COUNTDOWN) {
        g.countdown_ticks++;
        if (g.countdown_ticks >= TICKS_PER_SEC) {
            g.countdown_ticks = 0;

            if (g.countdown_seconds == 0) {
                g.data.turn.player     = 1;
                g.data.turn.cursor_col = 0;
                g.data.turn.cursor_row = 0;
                transition(STATE_TURN_P1);
                proto_send_state(STATE_TURN_P1);
            } else {
                g.countdown_seconds--;
                proto_send_countdown((uint8_t)g.countdown_seconds);
            }
        }
    }

    /* 3. Cursor sync during combat (throttled to 10 Hz) */
    static uint8_t cursor_tick = 0;
    cursor_tick++;
    if (cursor_tick >= 3) {
        cursor_tick = 0;

        /* HOST sends P1 cursor to CLIENT during P1's turn */
        if (g.role == ROLE_HOST && g.tag == STATE_TURN_P1) {
            proto_send_cursor(
                (uint8_t)g.data.turn.cursor_col,
                (uint8_t)g.data.turn.cursor_row);
        }

        /* CLIENT sends P2 cursor to HOST during P2's turn */
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

    /*
     * CLIENT routing:
     *   P2 placement / P2 attack turn → handle locally (fall through).
     *   Everything else               → forward to host via serial.
     */
    if (g.role == ROLE_CLIENT) {
        if (g.tag == STATE_PLACE_SHIPS_P2 || g.tag == STATE_TURN_P2) {
            /* fall through to local processing */
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

        /* ---- Main menu (HOST only) ---- */
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

        /* ---- Ship placement ---- */
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

                    /* CLIENT: send placed ship to host for p2_board sync */
                    if (g.role == ROLE_CLIENT) {
                        proto_send_ship_place(col, row, size, (uint8_t)orient);
                    }

                    g.data.place.ship_idx++;

                    if (g.data.place.ship_idx >= NUM_SHIPS) {
                        if (g.tag == STATE_PLACE_SHIPS_P1) {
                            /* HOST P1 done — tell client to start P2 placement */
                            g.data.place.ship_idx   = 0;
                            g.data.place.orient     = HORIZONTAL;
                            g.data.place.cursor_col = 0;
                            g.data.place.cursor_row = 0;
                            transition(STATE_PLACE_SHIPS_WAITING);
                            proto_send_state(STATE_PLACE_SHIPS_P2);
                        } else {
                            /* CLIENT P2 done — notify host */
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

        /* ---- Combat ---- */
        case STATE_TURN_P1:
        case STATE_TURN_P2: {
            /* Gate: only the active player fires */
            if (g.tag == STATE_TURN_P1 && g.role != ROLE_HOST)   break;
            if (g.tag == STATE_TURN_P2 && g.role != ROLE_CLIENT) break;

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
                        /* HOST applies locally, sends result to client */
                        bool hit = board_attack(enemy, col, row);
                        uint8_t result = ATTACK_MISS;
                        if (hit) {
                            result = ATTACK_HIT;
                            for (uint8_t i = 0; i < enemy->ships_placed; i++) {
                                if (enemy->ships[i].sunk &&
                                    enemy->ships[i].hits == enemy->ships[i].size) {
                                    result = ATTACK_SUNK;
                                    break;
                                }
                            }
                        }
                        proto_send_attack(col, row, result);

                        if (board_all_sunk(enemy)) {
                            uint8_t winner = (g.tag == STATE_TURN_P1) ? 1 : 2;
                            g.data.game_over.winner = winner;
                            transition(STATE_GAME_OVER);
                            proto_send_winner(winner);
                            proto_send_state(STATE_GAME_OVER);
                        } else {
                            g.data.turn.cursor_col = 0;
                            g.data.turn.cursor_row = 0;
                            transition(STATE_TURN_P2);
                            proto_send_state(STATE_TURN_P2);
                        }
                    } else {
                        /*
                         * CLIENT: do NOT apply locally.
                         * Send to host; host applies authoritatively,
                         * echoes MSG_ATTACK back, then sends MSG_STATE.
                         */
                        proto_send_attack(col, row, 0);
                    }
                }
            }

            if (!make && code == KEY_ESC)
                transition(STATE_PAUSED);
            break;
        }

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

    /*
     * CLIENT routing:
     *   STATE_PLACE_SHIPS_P2 → handle locally (fall through to switch).
     *   STATE_TURN_P2        → handle locally (fall through to switch).
     *   Everything else      → forward raw packet to host.
     */
    if (g.role == ROLE_CLIENT) {
        if (g.tag == STATE_PLACE_SHIPS_P2 || g.tag == STATE_TURN_P2) {
            /* fall through to switch below */
        } else {
            if (!g.connected) return;
            if (ms->moved || ms->clicked) {
                proto_send_mouse(get_mouse_buf());
            }
            return;
        }
    }

    int col, row;
    board_pixel_to_cell(ms->x, ms->y, &col, &row);

    switch (g.tag) {

        /* ---- Main menu (HOST only, CLIENT forwarded above) ---- */
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

        /* ---- Ship placement ---- */
        case STATE_PLACE_SHIPS_P1:
        case STATE_PLACE_SHIPS_P2: {
            /* Each role only handles their own board */
            if (g.tag == STATE_PLACE_SHIPS_P1 && g.role != ROLE_HOST)   break;
            if (g.tag == STATE_PLACE_SHIPS_P2 && g.role != ROLE_CLIENT) break;

            board_t *b = (g.tag == STATE_PLACE_SHIPS_P1)
                         ? &g.p1_board : &g.p2_board;

            if (col >= 0 && col < BOARD_COLS && row >= 0 && row < BOARD_ROWS) {
                g.data.place.cursor_col = col;
                g.data.place.cursor_row = row;
            }
            if (ms->clicked &&
                col >= 0 && col < BOARD_COLS &&
                row >= 0 && row < BOARD_ROWS) {
                uint8_t size = SHIP_SIZES[g.data.place.ship_idx];
                orientation_t orient = (orientation_t)g.data.place.orient;
                if (board_can_place(b, col, row, size, orient)) {
                    board_place_ship(b, col, row, size, orient);

                    /* CLIENT: sync ship to host */
                    if (g.role == ROLE_CLIENT) {
                        proto_send_ship_place((uint8_t)col, (uint8_t)row,
                                              size, (uint8_t)orient);
                    }

                    g.data.place.ship_idx++;
                    if (g.data.place.ship_idx >= NUM_SHIPS) {
                        if (g.tag == STATE_PLACE_SHIPS_P1) {
                            // Player 1 (Host) is finished. 
                            g.data.place.ship_idx   = 0;
                            g.data.place.orient     = HORIZONTAL;
                            g.data.place.cursor_col = 0;
                            g.data.place.cursor_row = 0;
                            
                            // Host safely goes to waiting room and commands client to start placing
                            transition(STATE_PLACE_SHIPS_WAITING);
                            proto_send_state(STATE_PLACE_SHIPS_P2);
                        } else {
                            // Player 2 (Client) is finished.
                            // FIX: We do NOT transition locally. We let the Host tell us 
                            // to move to STATE_COUNTDOWN authoritatively.
                            proto_send_done_placing();
                            printf("CLIENT sent MSG_DONE_PLACING\n");
                        }
                    }
                }
            }
            break;
        }

        /* ---- P1 attacks (HOST only) ---- */
        case STATE_TURN_P1: {
            if (g.role != ROLE_HOST) break;

            board_t *enemy = &g.p2_board;
            if (col >= 0 && col < BOARD_COLS && row >= 0 && row < BOARD_ROWS) {
                g.data.turn.cursor_col = col;
                g.data.turn.cursor_row = row;
            }
            if (ms->clicked &&
                col >= 0 && col < BOARD_COLS &&
                row >= 0 && row < BOARD_ROWS &&
                !board_already_attacked(enemy, col, row)) {

                bool hit = board_attack(enemy, col, row);
                uint8_t result = ATTACK_MISS;
                if (hit) {
                    result = ATTACK_HIT;
                    for (uint8_t i = 0; i < enemy->ships_placed; i++) {
                        if (enemy->ships[i].sunk &&
                            enemy->ships[i].hits == enemy->ships[i].size) {
                            result = ATTACK_SUNK;
                            break;
                        }
                    }
                }
                proto_send_attack((uint8_t)col, (uint8_t)row, result);

                if (board_all_sunk(enemy)) {
                    g.data.game_over.winner = 1;
                    transition(STATE_GAME_OVER);
                    proto_send_winner(1);
                    proto_send_state(STATE_GAME_OVER);
                } else {
                    g.data.turn.cursor_col = 0;
                    g.data.turn.cursor_row = 0;
                    transition(STATE_TURN_P2);
                    proto_send_state(STATE_TURN_P2);
                }
            }
            break;
        }

        /* ---- P2 attacks (CLIENT only) ---- */
        case STATE_TURN_P2: {
            if (g.role != ROLE_CLIENT) break;

            /* Update local cursor for rendering */
            if (col >= 0 && col < BOARD_COLS && row >= 0 && row < BOARD_ROWS) {
                g.data.turn.cursor_col = col;
                g.data.turn.cursor_row = row;
            }
            if (ms->clicked &&
                col >= 0 && col < BOARD_COLS &&
                row >= 0 && row < BOARD_ROWS &&
                !board_already_attacked(&g.p1_board, col, row)) {
                /*
                 * Do NOT apply locally — send to host.
                 * Host applies authoritatively, echoes MSG_ATTACK,
                 * then sends MSG_STATE to switch turns.
                 */
                proto_send_attack((uint8_t)col, (uint8_t)row, 0);
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
                printf("HOST received MSG_DONE_PLACING, tag=%d, WAITING=%d\n",
                g.tag, STATE_PLACE_SHIPS_WAITING);
                /* Remove the state guard — start countdown whenever client is done,
                * regardless of whether host is already in WAITING state */
                if (!g.connected) break;  /* safety only */
                g.countdown_seconds = COUNTDOWN_START;
                g.countdown_ticks   = 0;
                transition(STATE_COUNTDOWN);
                proto_send_state(STATE_COUNTDOWN);
                proto_send_countdown(g.countdown_seconds);
                break;

            case MSG_SHIP_PLACE: {
                /* P2 placed a ship — mirror into host's p2_board */
                uint8_t col  = msg->payload.ship.col;
                uint8_t row  = msg->payload.ship.row;
                uint8_t so   = msg->payload.ship.size_orient;
                uint8_t size = (so >> 4) & 0x0F;
                uint8_t ori  = so & 0x01;
                board_place_ship(&g.p2_board, col, row, size, (orientation_t)ori);
                break;
            }

            case MSG_KEY:
                /* Forward P2 keyboard only during P2's attack turn */
                if (g.tag == STATE_TURN_P2) {
                    game_role_t saved = g.role;
                    g.role = ROLE_HOST;
                    game_handle_keyboard(msg->payload.key.scancode);
                    g.role = saved;
                }
                break;

            case MSG_ATTACK: {
                /*
                 * CLIENT (P2) fired during STATE_TURN_P2.
                 * Host applies authoritatively to p1_board,
                 * echoes real result to client, then switches turn.
                 */
                if (g.tag != STATE_TURN_P2) break;

                uint8_t col    = msg->payload.attack.col;
                uint8_t row    = msg->payload.attack.row;
                board_t *enemy = &g.p1_board;

                if (board_already_attacked(enemy, col, row)) break;

                bool hit = board_attack(enemy, col, row);
                uint8_t result = ATTACK_MISS;
                if (hit) {
                    result = ATTACK_HIT;
                    for (uint8_t i = 0; i < enemy->ships_placed; i++) {
                        if (enemy->ships[i].sunk &&
                            enemy->ships[i].hits == enemy->ships[i].size) {
                            result = ATTACK_SUNK;
                            break;
                        }
                    }
                }

                /* Echo authoritative result BEFORE switching state */
                proto_send_attack(col, row, result);

                if (board_all_sunk(enemy)) {
                    g.data.game_over.winner = 2;
                    transition(STATE_GAME_OVER);
                    proto_send_winner(2);
                    proto_send_state(STATE_GAME_OVER);
                } else {
                    g.data.turn.cursor_col = 0;
                    g.data.turn.cursor_row = 0;
                    transition(STATE_TURN_P1);
                    proto_send_state(STATE_TURN_P1);
                }
                break;
            }

            case MSG_MOUSE:
                /*
                 * CLIENT mouse packet during P2's turn.
                 * Used for menu forwarding only — attacks now use MSG_ATTACK.
                 * During STATE_TURN_P2 we ignore mouse packets since
                 * the client sends MSG_ATTACK directly.
                 */
                if (g.tag == STATE_MAIN_MENU ||
                    g.tag == STATE_INSTRUCTIONS) {
                    uint8_t *rbuf = get_mouse_buf();
                    rbuf[0] = msg->payload.mouse.pkt[0];
                    rbuf[1] = msg->payload.mouse.pkt[1];
                    rbuf[2] = msg->payload.mouse.pkt[2];
                    mouse_state_update(get_mouse_state(), rbuf,
                                       video_get_hres(), video_get_vres());
                    game_handle_mouse(get_mouse_state());
                }
                break;

            case MSG_CURSOR:
                /* CLIENT cursor position during P2's turn */
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
            /* Also reset mouse flags (transition() does this,
             * but be explicit for clarity) */
            get_mouse_state()->clicked  = false;
            get_mouse_state()->released = false;
            get_mouse_state()->moved    = false;

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
             * Host sent an attack result.
             * STATE_TURN_P1 → P1 (host) attacked → update p2_board.
             * STATE_TURN_P2 → host echoing P2's attack → update p1_board.
             *
             * Use g.prev as fallback in case MSG_STATE arrived first
             * and already flipped g.tag.
             */
            uint8_t col = msg->payload.attack.col;
            uint8_t row = msg->payload.attack.row;

            game_state_t attack_state =
                (g.tag == STATE_TURN_P1 || g.tag == STATE_TURN_P2)
                ? g.tag : g.prev;

            board_t *enemy = (attack_state == STATE_TURN_P1)
                             ? &g.p2_board
                             : &g.p1_board;

            if (!board_already_attacked(enemy, col, row)) {
                board_attack(enemy, col, row);
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
/* game_draw                                                          */
/* ------------------------------------------------------------------ */
void game_draw(void) {
    video_clear_screen(0x000000);

    switch (g.tag) {

        case STATE_WAITING_CONNECT:
            if (g.role == ROLE_HOST) {
                draw_string("WAITING FOR", 248, 220, 0x00BFFF, 3);
                draw_string("PLAYER 2...", 248, 270, 0x00BFFF, 3);
                draw_string("(START CLIENT VM NOW)", 132, 380, 0x888888, 2);
            } else {
                draw_string("CONNECTING TO", 208, 220, 0xFFD700, 3);
                draw_string("HOST...", 304, 270, 0xFFD700, 3);
            }
            break;

        case STATE_MAIN_MENU:
            menu_draw_main(g.data.menu.selected);
            draw_rtc();
            break;

        case STATE_INSTRUCTIONS:
            menu_draw_instructions();
            break;

        case STATE_PLACE_SHIPS_P1:
            if (g.role == ROLE_HOST) {
                board_draw(&g.p1_board, false);
                board_draw_preview(&g.p1_board,
                    g.data.place.cursor_col, g.data.place.cursor_row,
                    SHIP_SIZES[g.data.place.ship_idx],
                    (orientation_t)g.data.place.orient);
                draw_hud_place(1, g.data.place.ship_idx);
            } else {
                draw_string("PLAYER 1 IS", 248, 220, 0x00BFFF, 3);
                draw_string("PLACING SHIPS...", 192, 270, 0x00BFFF, 3);
                draw_string("PLEASE WAIT", 248, 340, 0x888888, 2);
            }
            draw_rtc();
            break;

        case STATE_PLACE_SHIPS_P2:
            if (g.role == ROLE_CLIENT) {
                board_draw(&g.p2_board, false);
                board_draw_preview(&g.p2_board,
                    g.data.place.cursor_col, g.data.place.cursor_row,
                    SHIP_SIZES[g.data.place.ship_idx],
                    (orientation_t)g.data.place.orient);
                draw_hud_place(2, g.data.place.ship_idx);
            } else {
                draw_string("PLAYER 2 IS", 248, 220, 0x00BFFF, 3);
                draw_string("PLACING SHIPS...", 192, 270, 0x00BFFF, 3);
                draw_string("PLEASE WAIT", 248, 340, 0x888888, 2);
            }
            draw_rtc();
            break;

        case STATE_PLACE_SHIPS_WAITING:
            if (g.role == ROLE_HOST) {
                board_draw(&g.p1_board, false);
            } else {
                board_draw(&g.p2_board, false);
            }
            draw_string("WAITING FOR OPPONENT...", 160, 550, 0x888888, 2);
            draw_rtc();
            break;

        case STATE_COUNTDOWN: {
            draw_string("GAME STARTS IN", 192, 200, 0x00BFFF, 3);
            char digit[2] = { '0' + g.countdown_seconds, '\0' };
            draw_string(digit, 376, 280, 0xFFD700, 8);
            draw_rtc();
            break;
        }

        case STATE_TURN_P1: {
            /*
             * P1 (HOST) attacking P2's board.
             * HOST shows its own cursor (yellow).
             * CLIENT shows remote cursor (orange).
             */
            board_draw(&g.p2_board, true);
            if (g.role == ROLE_HOST) {
                board_highlight_cell(
                    g.data.turn.cursor_col,
                    g.data.turn.cursor_row);
            }
            if (g.role == ROLE_CLIENT && g.remote_cursor_col >= 0) {
                board_highlight_remote_cursor(
                    g.remote_cursor_col,
                    g.remote_cursor_row);
            }
            draw_hud_attack(1, &g.p2_board);
            draw_rtc();
            break;
        }

        case STATE_TURN_P2: {
            /*
             * P2 (CLIENT) attacking P1's board.
             * CLIENT shows its own cursor (yellow).
             * HOST shows remote cursor (orange).
             */
            board_draw(&g.p1_board, true);
            if (g.role == ROLE_CLIENT) {
                board_highlight_cell(
                    g.data.turn.cursor_col,
                    g.data.turn.cursor_row);
            }
            if (g.role == ROLE_HOST && g.remote_cursor_col >= 0) {
                board_highlight_remote_cursor(
                    g.remote_cursor_col,
                    g.remote_cursor_row);
            }
            draw_hud_attack(2, &g.p1_board);
            draw_rtc();
            break;
        }

        case STATE_PAUSED:
            menu_draw_pause(g.data.pause.selected);
            draw_rtc();
            break;

        case STATE_GAME_OVER:
            menu_draw_game_over(g.data.game_over.winner);
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
