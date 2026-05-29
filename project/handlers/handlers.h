#pragma once
/*
 * handlers.h — Interrupt handler declarations.
 *
 * CHANGE FROM ORIGINAL:
 *   Added handle_serial() declaration.
 *   Called from main.c when BIT(serial_bit) fires in the event loop.
 */
#include <stdint.h>

void handle_timer(void);
void handle_keyboard(void);
void handle_mouse(void);

/*
 * handle_serial() — NEW
 * Drains the UART RX ring buffer, feeds bytes to the protocol state
 * machine, and dispatches complete messages to game_handle_serial_msg().
 */
void handle_serial(void);
