#include "protocol.h"
#include "uart.h"

static uint8_t payload_len_for(uint8_t type) {
    switch (type) {
        case MSG_HELLO:         return MSG_HELLO_LEN;
        case MSG_HELLO_ACK:     return MSG_HELLO_ACK_LEN;
        case MSG_KEY:           return MSG_KEY_LEN;
        case MSG_MOUSE:         return MSG_MOUSE_LEN;
        case MSG_SHIP_PLACE:    return MSG_SHIP_PLACE_LEN;
        case MSG_ATTACK:        return MSG_ATTACK_LEN;
        case MSG_CURSOR:        return MSG_CURSOR_LEN;
        case MSG_STATE:         return MSG_STATE_LEN;
        case MSG_COUNTDOWN:     return MSG_COUNTDOWN_LEN;
        case MSG_WINNER:        return MSG_WINNER_LEN;
        case MSG_DONE_PLACING:  return MSG_DONE_PLACING_LEN;
        default:                return 0xFF;
    }
}

void proto_rx_reset(proto_rx_state_t *s) {
    s->have_type  = false;
    s->collected  = 0;
    s->expected   = 0;
    s->type       = 0;
}

bool proto_feed_byte(proto_rx_state_t *s, uint8_t b, serial_msg_t *msg_out) {
    if (!s->have_type) {
        uint8_t plen = payload_len_for(b);
        if (plen == 0xFF) return false;
        s->type      = b;
        s->expected  = plen;
        s->collected = 0;
        s->have_type = true;
        if (plen == 0) {
            s->have_type  = false;
            msg_out->type = s->type;
            return true;
        }
        return false;
    }

    /* store payload, if not complete skip this function call */
    s->buf[s->collected++] = b;
    if (s->collected < s->expected) return false;

    msg_out->type = s->type;

    /* fill serial_msg_t payload */
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
            msg_out->payload.ship.col          = s->buf[0];
            msg_out->payload.ship.row          = s->buf[1];
            msg_out->payload.ship.size         = s->buf[2];
            msg_out->payload.ship.type_orient  = s->buf[3];
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
            break;
    }

    s->have_type = false;
    s->collected = 0;
    return true;
}

void proto_send_hello(void)     { uart_send_byte(MSG_HELLO); }
void proto_send_hello_ack(void) { uart_send_byte(MSG_HELLO_ACK); }

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
                            uint8_t size, uint8_t type_idx, uint8_t orient) {

    /* type_idx fits in 3 bits (0-4), orient in 1 bit */
    uint8_t type_orient = (uint8_t)((type_idx << 1) | (orient & 0x01));
    uart_send_byte(MSG_SHIP_PLACE);
    uart_send_byte(col);
    uart_send_byte(row);
    uart_send_byte(size);
    uart_send_byte(type_orient);
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

void proto_send_done_placing(void) {
    uart_send_byte(MSG_DONE_PLACING);
}
