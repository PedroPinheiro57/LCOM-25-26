/*
 * handlers.c — Interrupt handlers called from the event loop in main.c.
 *
 * CHANGES FROM ORIGINAL:
 *   Added handle_serial():
 *     - Calls uart_ih() to drain the UART hardware and fill the
 *       software RX ring buffer (defined in uart.c).
 *     - Then pulls every complete byte out of the ring buffer and
 *       feeds it to proto_feed_byte() (the protocol state machine).
 *     - When proto_feed_byte() returns true (= full message assembled),
 *       calls game_handle_serial_msg() to let the game react.
 *
 *   handle_keyboard():
 *     - Unchanged in structure; the two-byte prefix assembler is kept.
 *     - game_handle_keyboard() now internally decides whether to
 *       process locally (HOST) or forward via serial (CLIENT).
 *
 *   handle_mouse():
 *     - Unchanged.  game_handle_mouse() decides HOST vs CLIENT logic.
 */

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
/* NEW: UART driver and application protocol */
#include "../serial/uart.h"
#include "../serial/protocol.h"

/* ------------------------------------------------------------------ */
/* handle_timer                                                       */
/* ------------------------------------------------------------------ */
void handle_timer(void) {
    timer_int_handler();   /* acknowledge the timer interrupt in lcf  */
    game_handle_timer();   /* update RTC, countdown, send cursor      */
}

/* ------------------------------------------------------------------ */
/* handle_keyboard                                                    */
/* ------------------------------------------------------------------ */
/*
 * Two-byte scancode assembler (unchanged from original).
 *
 * Arrow keys and some other special keys send a 0xE0 prefix byte
 * followed by the actual key byte.  kbc_ih() fires once per byte, so
 * handle_keyboard() is called twice for those keys.
 *
 * We swallow the 0xE0 prefix and pass the second byte directly to
 * game_handle_keyboard().  The KEY_* constants (KEY_UP = 0x48, etc.)
 * match the second byte, so no adjustment is needed.
 *
 * game_handle_keyboard() now internally decides:
 *   HOST  → process locally (run game logic)
 *   CLIENT → call proto_send_key() to forward over serial
 */
static bool expecting_second_byte = false;

void handle_keyboard(void) {
    kbc_ih();
    if (kbc_has_error()) return;

    uint8_t sc = kbc_get_scancode_byte();

    if (sc == TWOBYTE_PREFIX) {
        /* 0xE0 prefix — wait for the second byte */
        expecting_second_byte = true;
        return;
    }

    expecting_second_byte = false;
    game_handle_keyboard(sc);
}

/* ------------------------------------------------------------------ */
/* handle_mouse                                                       */
/* ------------------------------------------------------------------ */
/*
 * Assembles the 3-byte PS/2 mouse packet exactly as before.
 *
 * Once a complete packet is ready, mouse_state_update() is called.
 * Then game_handle_mouse() decides:
 *   HOST  → process locally (update cursor, check clicks)
 *   CLIENT → call proto_send_mouse() to forward over serial
 *
 * NOTE for CLIENT: we still update the local mouse_state so the
 * cursor sprite is drawn at the correct position on the client screen.
 */
void handle_mouse(void) {
    mouse_ih();
    if (mouse_has_error()) return;

    uint8_t byte = mouse_get_byte();
    uint8_t idx  = get_mouse_idx();

    /* Byte 0 must have the sync bit set; discard desynchronised bytes */
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
