#include <lcom/lcf.h>
#include <lcom/timer.h>
#include <string.h>              
#include "../pedro/lab2/i8254.h"
#include "../pedro/lab3/kbc.h"
#include "../pedro/lab4/mouse.h"
#include "../pedro/lab5/video.h"
#include "model/mouse.h"
#include "controller/handlers.h"
#include "controller/game.h"
#include "view/sprites.h"
#include "serial/uart.h"
#include "serial/protocol.h"
#include "view/renderer.h"


extern int foo();

/* IRQ hook bit indices returned by the subscribe functions           */
static uint8_t timer_bit;
static uint8_t kbd_bit;
static uint8_t mouse_bit;
static uint8_t serial_bit;  

game_role_t role = ROLE_HOST;
bool role_is_client() {
    return role == ROLE_CLIENT;
}

/* ------------------------------------------------------------------ */
/* devices_init                                                       */
/* ------------------------------------------------------------------ */
static int devices_init(void) {
    if (video_init(0x115) != 0) return 1;

    /* Timer */
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

    /* Serial Port */
    if (uart_init(&serial_bit) != 0) {
        mouse_unsubscribe_int();
        kbc_unsubscribe_int();
        timer_unsubscribe_int();
        return 1;
    }

    /* Init mouse struct in the centre of the screen               */
    mouse_state_init(get_mouse_state(), 400, 300);
    return 0;
}

/* ------------------------------------------------------------------ */
/* devices_cleanup                                                    */
/* ------------------------------------------------------------------ */
static int devices_cleanup(void) {
    uart_cleanup();              
    if (mouse_unsubscribe_int() != 0)           return 1;
    if (kbc_unsubscribe_int() != 0)             return 1;
    if (mouse_disable_data_reporting() != 0)    return 1;
    if (timer_unsubscribe_int() != 0)           return 1;
    vg_exit();
    return 0;
}

/* ------------------------------------------------------------------ */
/* proj_main_loop                                                     */
/* ------------------------------------------------------------------ */
int(proj_main_loop)(int argc, char *argv[]) {
    foo();

    /* ---- Parse role from command-line argument ---- */
    /*
     * Expected usage:
     *   lcom_run proj "host"    → original VM runs game logic
     *   lcom_run proj "client"  → cloned VM forwards input
     */
    if (argc == 1) {
        if (strcmp(argv[0], "client") == 0) {
            printf("running as client\n");
            role = ROLE_CLIENT;
        } else if (strcmp(argv[0], "host") == 0) {
            role = ROLE_HOST;
            printf("running as host\n");
        }
    }

    /* ---- Initialise devices ---- */
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

        /* Serial interrupt */
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

        /* Timer interrupt */
        if (irqs & BIT(timer_bit)) {
            handle_timer();
        }
    }

    if (devices_cleanup() != 0) return 1;
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
