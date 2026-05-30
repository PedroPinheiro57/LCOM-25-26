#pragma once
/*
 * protocol.h — Application-level message protocol on top of the UART.
 */

#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/* Message type tags                                                  */
/* ------------------------------------------------------------------ */
#define MSG_HELLO          'H'
#define MSG_HELLO_ACK      'A'
#define MSG_KEY            'K'
#define MSG_MOUSE          'M'
#define MSG_SHIP_PLACE     'P'
#define MSG_ATTACK         'T'
#define MSG_CURSOR         'C'
#define MSG_STATE          'S'
#define MSG_COUNTDOWN      'D'
#define MSG_WINNER         'W'
#define MSG_DONE_PLACING   'F'   /* client → host: P2 finished placing ships */

/* ------------------------------------------------------------------ */
/* Payload sizes                                                      */
/* ------------------------------------------------------------------ */
#define MSG_HELLO_LEN          0
#define MSG_HELLO_ACK_LEN      0
#define MSG_KEY_LEN            1
#define MSG_MOUSE_LEN          3
#define MSG_SHIP_PLACE_LEN     3
#define MSG_ATTACK_LEN         3
#define MSG_CURSOR_LEN         2
#define MSG_STATE_LEN          1
#define MSG_COUNTDOWN_LEN      1
#define MSG_WINNER_LEN         1
#define MSG_DONE_PLACING_LEN   0

#define MSG_MAX_PAYLOAD        3

/* ------------------------------------------------------------------ */
/* Attack result codes                                                */
/* ------------------------------------------------------------------ */
#define ATTACK_MISS   0
#define ATTACK_HIT    1
#define ATTACK_SUNK   2

/* ------------------------------------------------------------------ */
/* Parsed message structure                                           */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t type;
    union {
        struct { uint8_t scancode; }      key;
        struct { uint8_t pkt[3]; }        mouse;
        struct { uint8_t col, row, size_orient; } ship;
        struct { uint8_t col, row, result; }      attack;
        struct { uint8_t col, row; }      cursor;
        struct { uint8_t state; }         state;
        struct { uint8_t seconds; }       countdown;
        struct { uint8_t winner; }        winner;
    } payload;
} serial_msg_t;

/* ------------------------------------------------------------------ */
/* Receiver state machine                                             */
/* ------------------------------------------------------------------ */
typedef struct {
    uint8_t type;
    uint8_t buf[MSG_MAX_PAYLOAD];
    uint8_t collected;
    uint8_t expected;
    bool    have_type;
} proto_rx_state_t;

void proto_rx_reset(proto_rx_state_t *s);
bool proto_feed_byte(proto_rx_state_t *s, uint8_t b, serial_msg_t *msg_out);

/* ------------------------------------------------------------------ */
/* Sender helpers                                                     */
/* ------------------------------------------------------------------ */
void proto_send_hello(void);
void proto_send_hello_ack(void);
void proto_send_key(uint8_t scancode);
void proto_send_mouse(uint8_t pkt[3]);
void proto_send_ship_place(uint8_t col, uint8_t row, uint8_t size, uint8_t orient);
void proto_send_attack(uint8_t col, uint8_t row, uint8_t result);
void proto_send_cursor(uint8_t col, uint8_t row);
void proto_send_state(uint8_t state);
void proto_send_countdown(uint8_t seconds);
void proto_send_winner(uint8_t winner);
void proto_send_done_placing(void);
