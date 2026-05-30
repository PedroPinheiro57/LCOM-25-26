/*
 * main.c — Entry point and event loop.
 *
 * CHANGES FROM ORIGINAL:
 *
 *   1. Role parsing from argv:
 *        lcom_run proj -args "host"    → ROLE_HOST
 *        lcom_run proj -args "client"  → ROLE_CLIENT
 *      The role is passed to game_init().
 *
 *   2. UART initialisation in devices_init():
 *      uart_init() is called, subscribing COM1 (IRQ4) in exclusive mode.
 *      The returned hook id is stored in serial_bit.
 *
 *   3. CLIENT sends MSG_HELLO immediately after uart_init(), before the
 *      event loop starts.  The HOST will see this and reply with
 *      MSG_HELLO_ACK, completing the handshake.
 *
 *   4. Event loop now also checks BIT(serial_bit):
 *      When IRQ4 fires, handle_serial() is called.
 *
 *   5. uart_cleanup() is called in devices_cleanup().
 *
 * HOW TO RUN:
 *   On the HOST VM (original):   lcom_run proj -args "host"
 *   On the CLIENT VM (clone):    lcom_run proj -args "client"
 *   Start the HOST first, then the CLIENT (required by VirtualBox TCP
 *   socket — see LCOM_-_Project__Serial_Communication_Testing.pdf).
 */

#include <lcom/lcf.h>
#include <lcom/timer.h>
#include <string.h>              /* strcmp() for role parsing         */
#include "../pedro/lab2/i8254.h"
#include "../pedro/lab3/kbc.h"
#include "../pedro/lab4/mouse.h"
#include "../pedro/lab5/video.h"
#include "devices/mouse.h"
#include "handlers/handlers.h"
#include "game/game.h"
#include "video/sprites.h"
/* NEW: UART and protocol headers */
#include "serial/uart.h"
#include "serial/protocol.h"
#include "game/renderer.h"

extern int foo();
int timer_get_counter(void);

/* IRQ hook bit indices returned by the subscribe functions           */
static uint8_t timer_bit;
static uint8_t kbd_bit;
static uint8_t mouse_bit;
static uint8_t serial_bit;   /* NEW: COM1 (IRQ4) hook bit            */

/* ------------------------------------------------------------------ */
/* devices_init                                                       */
/* ------------------------------------------------------------------ */
static int devices_init(void) {
    /* Video mode 0x115 = 800x600 direct colour */
    if (video_init(0x115) != 0) return 1;

    /* Timer at 30 Hz — drives the game loop and countdown          */
    if (timer_set_frequency(0, 30) != 0) return 1;
    if (timer_subscribe_int(&timer_bit) != 0) return 1;

    /* Keyboard */
    if (kbc_subscribe_int(&kbd_bit) != 0) {
        timer_unsubscribe_int();
        return 1;
    }

    /* Mouse */
    if (mouse_subscribe_int(&mouse_bit) != 0) {
        kbc_unsubscribe_int();
        timer_unsubscribe_int();
        return 1;
    }
    if (mouse_enable_data_reporting() != 0) {
        mouse_unsubscribe_int();
        kbc_unsubscribe_int();
        timer_unsubscribe_int();
        return 1;
    }

    /* NEW: Serial port (COM1, IRQ4, exclusive mode, 9600-8-N-1)    */
    if (uart_init(&serial_bit) != 0) {
        mouse_unsubscribe_int();
        kbc_unsubscribe_int();
        timer_unsubscribe_int();
        return 1;
    }

    /* Start mouse cursor in the centre of the screen               */
    mouse_state_init(get_mouse_state(), 400, 300);
    return 0;
}

/* ------------------------------------------------------------------ */
/* devices_cleanup                                                    */
/* ------------------------------------------------------------------ */
static void devices_cleanup(void) {
    uart_cleanup();               /* NEW: disable UART interrupts, unsubscribe */
    mouse_unsubscribe_int();
    kbc_unsubscribe_int();
    mouse_disable_data_reporting();
    timer_unsubscribe_int();
    vg_exit();
}

/* ------------------------------------------------------------------ */
/* proj_main_loop                                                     */
/* ------------------------------------------------------------------ */
int(proj_main_loop)(int argc, char *argv[]) {
    foo();

    /* ---- Parse role from command-line argument ---- */
    /*
     * Expected usage:
     *   lcom_run proj -args "host"    → original VM runs game logic
     *   lcom_run proj -args "client"  → cloned VM forwards input
     *
     * Default to HOST if no argument is given (safe for single testing).
     */
    game_role_t role = ROLE_HOST;   /* default */
    if (argc >= 1) {
        if (strcmp(argv[0], "client") == 0) {
            printf("running as client\n");
            role = ROLE_CLIENT;
        } else if (strcmp(argv[0], "host") == 0) {
            role = ROLE_HOST;
            printf("running as host\n");
        }
        /* Any other argument → default HOST */
    }

    /* ---- Initialise all hardware ---- */
    if (devices_init() != 0) return 1;

    /* ---- Initialise game state with the chosen role ---- */
    game_init(role);

    /* ---- Main event loop ---- */
    int r, ipc_status;
    message msg;

    while (!game_is_over()) {
        if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) continue;
        if (!is_ipc_notify(ipc_status)) continue;
        if (_ENDPOINT_P(msg.m_source) != HARDWARE) continue;

        uint32_t irqs = msg.m_notify.interrupts;

        /*
         * Dispatch interrupts.
         *
         * Order matters slightly: we handle serial BEFORE the timer
         * so that any state update received over serial is applied
         * before game_draw() renders the frame at the end of the
         * timer block.  This reduces perceived latency by one frame.
         */

        /* NEW: Serial port interrupt (COM1, IRQ4) */
        if (irqs & BIT(serial_bit)) {
            handle_serial();
        }

        /* Mouse interrupt */
        if (irqs & BIT(mouse_bit)) {
            handle_mouse();
        }

        /* Keyboard interrupt */
        if (irqs & BIT(kbd_bit)) {
            handle_keyboard();
        }

        /* Timer interrupt — render at 30 Hz */
        if (irqs & BIT(timer_bit)) {
            handle_timer();
            update_animations(); 

            /* CLIENT: retry MSG_HELLO once per second until connected */
            if (role == ROLE_CLIENT && game_is_waiting_connect() && !game_is_connected()) {
                if (timer_get_counter() % 30 == 0) {
                    proto_send_hello();
                    printf("Client sending MSG_HELLO...\n");
                }
            }

            /*
             * Only process local mouse input when it is the local
             * player's turn.  During P2's states on the HOST, input
             * arrives via serial (MSG_MOUSE) and is handled in
             * game_handle_serial_msg — calling game_handle_mouse with
             * the host's own mouse here would clobber P2's cursor.
             */
            if (!game_is_client_turn()) {
                game_handle_mouse(get_mouse_state());
            }

            video_clear_screen(0x000000);
            game_draw();
            cursor_draw(get_mouse_state()->x, get_mouse_state()->y);
            video_swap_buffers();
        }
    }

    devices_cleanup();
    return 0;
}

/* ------------------------------------------------------------------ */
/* main — lcf entry point (unchanged)                                */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[]) {
    lcf_set_language("EN-US");
    lcf_trace_calls("/home/lcom/labs/grupo_2leic01_5/project/trace.txt");
    lcf_log_output("/home/lcom/labs/grupo_2leic01_5/project/output.txt");
    if (lcf_start(argc, argv))
        return 1;
    lcf_cleanup();
    return 0;
}
