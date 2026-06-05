#include "handlers.h"
#include "../model/mouse.h"
#include <lcom/lcf.h>
#include "../../pedro/lab3/kbc.h"
#include "../../pedro/lab4/mouse.h"
#include "../model/keyboard.h"
#include "../model/mouse.h"
#include "../../pedro/lab5/video.h"
#include "../view/sprites.h"
#include "../controller/game.h"
#include "../serial/uart.h"
#include "../serial/protocol.h"
#include "../view/renderer.h"
#include "../main.h"

int timer_get_counter(); /* required for compiling */


void handle_timer(void) {
    timer_int_handler();
    game_handle_timer();

    update_animations();
    game_draw(game_get_state());
    cursor_draw(get_mouse_state()->x, get_mouse_state()->y);
    video_swap_buffers();

    /* try connect the 2 VMs every second if initial attempt when switching states didnt work */
    if (role_is_client() && game_is_waiting_connect()) {
        if (timer_get_counter() % 30 == 0) {
            proto_send_hello();
            printf("Client sending MSG_HELLO\n");
        }
    }
}



void handle_keyboard(void) {
    kbc_ih();
    if (kbc_has_error()) return;

    uint8_t sc = kbc_get_scancode_byte();
    if (sc == TWOBYTE_PREFIX) return;
    game_handle_keyboard(sc);
}



void handle_mouse(void) {
    mouse_ih();
    if (mouse_has_error()) return;

    uint8_t byte = mouse_get_byte();
    uint8_t idx  = get_mouse_idx();

    /* Byte 0 must have the sync bit set — drop desynced bytes */
    if (idx == 0 && !(byte & MOUSE_SYNC_BIT)) return;

    get_mouse_buf()[idx++] = byte;
    set_mouse_idx(idx);

    /* full mouse packet */
    if (idx == 3) {
        mouse_state_update(get_mouse_state(), get_mouse_buf(),
                           video_get_hres(), video_get_vres());
        set_mouse_idx(0);
        if (!game_is_client_turn()) {
            game_handle_mouse(get_mouse_state());
            /* restore mouse flags */
            get_mouse_state()->clicked     = false;
            get_mouse_state()->moved       = false;
            get_mouse_state()->released    = false;
        }
    }
}


static proto_rx_state_t rx_state;
proto_rx_state_t *get_rx_state(void) {
    return &rx_state;
}

void handle_serial(void) {
    uart_ih();

    if (uart_rx_overflow()) {
        proto_rx_reset(&rx_state);
        return;
    }

    uint8_t b;
    while (uart_recv_byte(&b)) {
        serial_msg_t msg;
        if (proto_feed_byte(&rx_state, b, &msg)) {
            game_handle_serial_msg(&msg);
        }
    }
}
