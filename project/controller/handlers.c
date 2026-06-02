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

int timer_get_counter();

/* ------------------------------------------------------------------ */
/* handle_timer                                                       */
/* ------------------------------------------------------------------ */
void handle_timer(void) {
    timer_int_handler();   
    game_handle_timer();   /* update game */

    update_animations(); 

    /* Only process player 1 mouse, player 2 mouse is handled with the serial port */
    if (!game_is_client_turn()) {
        game_handle_mouse(get_mouse_state());
    }

    video_clear_screen();
    game_draw(game_get_state());
    cursor_draw(get_mouse_state()->x, get_mouse_state()->y);
    video_swap_buffers();

    /* CLIENT: retry MSG_HELLO once per second until connected */
    if (role_is_client() && game_is_waiting_connect() && !game_is_connected()) {
        if (timer_get_counter() % 30 == 0) {
            proto_send_hello();
            printf("Client sending MSG_HELLO...\n");
        }
    }
}

/* ------------------------------------------------------------------ */
/* handle_keyboard                                                    */
/* ------------------------------------------------------------------ */
void handle_keyboard(void) {
    kbc_ih();
    if (kbc_has_error()) return;

    uint8_t sc = kbc_get_scancode_byte();
    if (sc == TWOBYTE_PREFIX) return;
    game_handle_keyboard(sc);
}

/* ------------------------------------------------------------------ */
/* handle_mouse                                                       */
/* ------------------------------------------------------------------ */
void handle_mouse(void) {
    mouse_ih();
    if (mouse_has_error()) return;

    uint8_t byte = mouse_get_byte();
    uint8_t idx  = get_mouse_idx();

    /* Byte 0 must have the sync bit (3) set */
    if (idx == 0 && !(byte & MOUSE_SYNC_BIT)) return;

    get_mouse_buf()[idx++] = byte;
    set_mouse_idx(idx);

    /* Full packet assembled */
    if (idx == 3) {
        mouse_state_update(get_mouse_state(), get_mouse_buf(), video_get_hres(), video_get_vres());
        set_mouse_idx(0);
    }
}


static proto_rx_state_t rx_state;

proto_rx_state_t *get_rx_state() {
    return &rx_state;
}

void handle_serial(void) {
    uart_ih();

    uint8_t b;
    while (uart_recv_byte(&b)) {
        serial_msg_t msg;
        if (proto_feed_byte(&rx_state, b, &msg)) {
            game_handle_serial_msg(&msg);
        }
    }
}
