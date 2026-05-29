/*
 * protocol.c — Application-level message protocol implementation.
 *
 * See protocol.h for the full design explanation.
 */

#include "protocol.h"
#include "uart.h"   /* uart_send_byte() */

/* ------------------------------------------------------------------ */
/* Helper: how many payload bytes does a given type expect?          */
/* ------------------------------------------------------------------ */
/*
 * Returns the number of payload bytes (NOT counting the type byte)
 * for the given message type.  Returns 0xFF for unknown types so
 * the state machine can drop them gracefully.
 */
static uint8_t payload_len_for(uint8_t type) {
    switch (type) {
        case MSG_HELLO:      return MSG_HELLO_LEN;
        case MSG_HELLO_ACK:  return MSG_HELLO_ACK_LEN;
        case MSG_KEY:        return MSG_KEY_LEN;
        case MSG_MOUSE:      return MSG_MOUSE_LEN;
        case MSG_SHIP_PLACE: return MSG_SHIP_PLACE_LEN;
        case MSG_ATTACK:     return MSG_ATTACK_LEN;
        case MSG_CURSOR:     return MSG_CURSOR_LEN;
        case MSG_STATE:      return MSG_STATE_LEN;
        case MSG_COUNTDOWN:  return MSG_COUNTDOWN_LEN;
        case MSG_WINNER:     return MSG_WINNER_LEN;
        case MSG_DONE_PLACING:  return MSG_DONE_PLACING_LEN;  /* ← add this */
        default:             return 0xFF;   /* unknown — discard      */
    }
}

/* ------------------------------------------------------------------ */
/* proto_rx_reset                                                     */
/* ------------------------------------------------------------------ */
void proto_rx_reset(proto_rx_state_t *s) {
    s->have_type  = false;
    s->collected  = 0;
    s->expected   = 0;
    s->type       = 0;
}

/* ------------------------------------------------------------------ */
/* proto_feed_byte                                                    */
/* ------------------------------------------------------------------ */
/*
 * This is a two-phase state machine:
 *
 *   Phase 1 — waiting for the type byte (have_type == false):
 *     We read the first byte, look up the expected payload length,
 *     and if it's a known type we move to phase 2.
 *     If the payload length is 0 (e.g. MSG_HELLO), the message is
 *     immediately complete and we return true right away.
 *
 *   Phase 2 — collecting payload bytes (have_type == true):
 *     We accumulate bytes into buf[] until collected == expected,
 *     then decode the payload into msg_out and return true.
 *
 * If we see an unknown type byte we reset and wait for the next one.
 * This gives us basic resynchronization as recommended by the slides.
 */
bool proto_feed_byte(proto_rx_state_t *s, uint8_t b, serial_msg_t *msg_out) {

    if (!s->have_type) {
        uint8_t plen = payload_len_for(b);
        
        if (plen == 0xFF) {
            printf("ERROR: Unknown Type 0x%02x\n", b);
            return false;
        }

        s->type      = b;
        s->expected  = plen;
        s->collected = 0;
        s->have_type = true;

        /* Zero-payload messages are complete right now */
        if (plen == 0) {
            s->have_type     = false;   /* reset for next message */
            msg_out->type    = s->type;
            return true;
        }
        return false;
    }

    /* ---- Phase 2: collect payload bytes ---- */
    s->buf[s->collected++] = b;

    if (s->collected < s->expected) {
        /* Still waiting for more bytes */
        return false;
    }

    /* All payload bytes received — decode into msg_out */
    msg_out->type = s->type;

    switch (s->type) {

        case MSG_KEY:
            msg_out->payload.key.scancode = s->buf[0];
            break;

        case MSG_MOUSE:
            msg_out->payload.mouse.pkt[0] = s->buf[0];
            msg_out->payload.mouse.pkt[1] = s->buf[1];
            msg_out->payload.mouse.pkt[2] = s->buf[2];
            break;

        case MSG_SHIP_PLACE:
            msg_out->payload.ship.col         = s->buf[0];
            msg_out->payload.ship.row         = s->buf[1];
            msg_out->payload.ship.size_orient = s->buf[2];
            break;

        case MSG_ATTACK:
            msg_out->payload.attack.col    = s->buf[0];
            msg_out->payload.attack.row    = s->buf[1];
            msg_out->payload.attack.result = s->buf[2];
            break;

        case MSG_CURSOR:
            msg_out->payload.cursor.col = s->buf[0];
            msg_out->payload.cursor.row = s->buf[1];
            break;

        case MSG_STATE:
            msg_out->payload.state.state = s->buf[0];
            break;

        case MSG_COUNTDOWN:
            msg_out->payload.countdown.seconds = s->buf[0];
            break;

        case MSG_WINNER:
            msg_out->payload.winner.winner = s->buf[0];
            break;

        default:
            /* Should not reach here — unknown types filtered in phase 1 */
            break;
    }

    /* Reset for the next message */
    s->have_type = false;
    s->collected = 0;

    return true;
}

/* ------------------------------------------------------------------ */
/* Sender helpers                                                     */
/* ------------------------------------------------------------------ */
/*
 * Each function calls uart_send_byte() for the type tag byte first,
 * then for every payload byte in order.
 * uart_send_byte() queues bytes; actual transmission happens via
 * the UART TX interrupt (see uart.c).
 */

void proto_send_hello(void) {
    uart_send_byte(MSG_HELLO);
}


void proto_send_hello_ack(void) {
    uart_send_byte(MSG_HELLO_ACK);
    /* No payload */
}

void proto_send_key(uint8_t scancode) {
    uart_send_byte(MSG_KEY);
    uart_send_byte(scancode);
}

void proto_send_mouse(uint8_t pkt[3]) {
    uart_send_byte(MSG_MOUSE);
    uart_send_byte(pkt[0]);
    uart_send_byte(pkt[1]);
    uart_send_byte(pkt[2]);
}

void proto_send_ship_place(uint8_t col, uint8_t row,
                            uint8_t size, uint8_t orient) {
    /*
     * Pack size (4 bits) and orient (1 bit) into one byte:
     *   bits 7:4 = size (values 1-5 fit in 4 bits)
     *   bit  0   = orient (0 = HORIZONTAL, 1 = VERTICAL)
     */
    uint8_t size_orient = (uint8_t)((size << 4) | (orient & 0x01));
    uart_send_byte(MSG_SHIP_PLACE);
    uart_send_byte(col);
    uart_send_byte(row);
    uart_send_byte(size_orient);
}

void proto_send_attack(uint8_t col, uint8_t row, uint8_t result) {
    uart_send_byte(MSG_ATTACK);
    uart_send_byte(col);
    uart_send_byte(row);
    uart_send_byte(result);
}

void proto_send_cursor(uint8_t col, uint8_t row) {
    uart_send_byte(MSG_CURSOR);
    uart_send_byte(col);
    uart_send_byte(row);
}

void proto_send_state(uint8_t state) {
    uart_send_byte(MSG_STATE);
    uart_send_byte(state);
}

void proto_send_countdown(uint8_t seconds) {
    uart_send_byte(MSG_COUNTDOWN);
    uart_send_byte(seconds);
}

void proto_send_winner(uint8_t winner) {
    uart_send_byte(MSG_WINNER);
    uart_send_byte(winner);
}


// protocol.c
void proto_send_done_placing(void) {
    uart_send_byte(MSG_DONE_PLACING);
}
