/**
 * @file handlers.h
 * @brief Top-level interrupt and event dispatch stubs.
 *
 * Each function is called from the main event loop whenever the
 * corresponding interrupt bit fires.  They read raw hardware data,
 * update the relevant device model (keyboard, mouse, UART), and then
 * forward the decoded event to the game controller via the
 * @ref game_handle_* family of functions.
 *
 * These are thin glue routines; all game logic lives in
 * @ref controller/game.h.
 */

#pragma once
#include <stdint.h>

/**
 * @brief Handles a hardware timer interrupt.
 *
 * Acknowledges the interrupt and calls @ref game_handle_timer to advance
 * the game clock by one tick.
 */
void handle_timer(void);

/**
 * @brief Handles a keyboard (KBC) interrupt.
 *
 * Reads the scancode from the KBC output buffer and forwards it to
 * @ref game_handle_keyboard.  On the client VM the scancode is also
 * serialised and sent to the host via @ref proto_send_key.
 */
void handle_keyboard(void);

/**
 * @brief Handles a PS/2 mouse interrupt.
 *
 * Accumulates incoming PS/2 bytes into the three-byte packet buffer.
 * When a complete packet is ready it calls @ref mouse_state_update and
 * then @ref game_handle_mouse.  On the client VM the raw packet is also
 * forwarded to the host via @ref proto_send_mouse.
 */
void handle_mouse(void);

/**
 * @brief Handles a UART (serial) receive interrupt.
 *
 * Calls @ref uart_ih to drain the hardware FIFO into the software ring
 * buffer, then feeds each byte through @ref proto_feed_byte.  When a
 * complete message is assembled it is dispatched to
 * @ref game_handle_serial_msg.
 */
void handle_serial(void);
