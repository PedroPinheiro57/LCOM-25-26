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
}

/* ------------------------------------------------------------------ */
/* handle_keyboard                                                    */
/* ------------------------------------------------------------------ */
/*
 * game_handle_keyboard() now internally decides:
 *   HOST  → process locally (run game logic)
 *   CLIENT → call proto_send_key() to forward over serial
 */

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
/* Once a complete packet is ready, mouse_state_update() is called.
 * Then game_handle_mouse() decides:
 *   HOST  → process locally (update cursor, check clicks)
 *   CLIENT → call proto_send_mouse() to forward over serial
 */

void handle_mouse(void) {
    mouse_ih();
    if (mouse_has_error()) return;

    uint8_t byte = mouse_get_byte();
    uint8_t idx  = get_mouse_idx();

    /* Byte 0 must have the sync bit (3) set */
    if (idx == 0 && !(byte & MOUSE_SYNC_BIT)) return;

    get_mouse_buf()[idx++] = byte;
    set_mouse_idx(idx);

    if (idx == 3) {
        /* Full packet assembled */
        mouse_state_update(get_mouse_state(), get_mouse_buf(),
                           video_get_hres(), video_get_vres());
        set_mouse_idx(0);
        /* game_handle_mouse handles HOST vs CLIENT internally */
    }
}

/* ------------------------------------------------------------------ */
/* handle_serial  (NEW)                                               */
/* ------------------------------------------------------------------ */
/*
 * Called from the event loop in main.c when an IRQ4 (COM1) interrupt
 * fires (i.e., when irqs & BIT(serial_bit) is true).
 *
 * Steps:
 *   1. Call uart_ih() — this drains the hardware UART/FIFO and fills
 *      the software RX ring buffer with received raw bytes.
 *      It also handles TX interrupts (sends next queued byte).
 *
 *   2. Pull every available byte from the RX ring buffer using
 *      uart_recv_byte() and feed them one by one into proto_feed_byte().
 *
 *   3. When proto_feed_byte() returns true, a complete application
 *      message has been assembled.  Pass it to game_handle_serial_msg()
 *      which applies it to the game state (HOST or CLIENT logic).
 *
 * We loop until the RX ring buffer is empty so that a burst of bytes
 * (e.g., a full mouse packet arriving in one interrupt) is processed
 * entirely in this handler invocation.
 */

/* Module-level receiver state machine — persists between interrupts  */
static proto_rx_state_t rx_state;
static bool rx_state_initialised = false;

void handle_serial(void) {

    if (!rx_state_initialised) {
        proto_rx_reset(&rx_state);
        rx_state_initialised = true;
    }

    uart_ih();

    uint8_t b;
    while (uart_recv_byte(&b)) {
        serial_msg_t msg;
        if (proto_feed_byte(&rx_state, b, &msg)) {
            game_handle_serial_msg(&msg);
        }
    }
}
