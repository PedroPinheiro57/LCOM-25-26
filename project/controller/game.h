/**
 * @file game.h
 * @brief Central game-state machine: structs, state tags, and controller API.
 *
 * The game is split across two VMs.  The @e host owns the authoritative
 * game state and drives the logic; the @e client forwards raw input over
 * the serial link and renders whatever the host reports back.
 *
 * All state transitions happen through the functions declared here.
 * External code should treat @ref game_t as read-only and access it only
 * via @ref game_get_state.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../model/mouse.h"
#include "../model/rtc.h"
#include "../serial/protocol.h"
#include "../model/board.h"

/** @brief Timer ticks generated per second by the hardware timer. */
#define TICKS_PER_SEC          30
/** @brief Seconds shown on the pre-combat countdown screen. */
#define COUNTDOWN_START        5
/** @brief Ticks to wait after an attack animation before resuming input. */
#define POST_ATTACK_WAIT_TICKS 60

/* ------------------------------------------------------------------ */
/* Role                                                                */
/* ------------------------------------------------------------------ */

/**
 * @brief Identifies which of the two VMs this instance is running as.
 *
 * Set once at startup and never changed during a session.
 */
typedef enum {
    ROLE_HOST,   /**< Original VM: runs all game logic, sends state to the client. */
    ROLE_CLIENT  /**< Cloned VM: forwards input upstream, renders from host data.  */
} game_role_t;

/* ------------------------------------------------------------------ */
/* Game states                                                         */
/* ------------------------------------------------------------------ */

/**
 * @brief All possible states the game state machine can be in.
 *
 * The state drives both what is rendered on screen and which input
 * events are meaningful at any given moment.
 */
typedef enum {
    STATE_MAIN_MENU,         /**< Startup menu (Play / Instructions / Exit).        */
    STATE_INSTRUCTIONS,      /**< Instructions / help screen.                       */
    STATE_WAITING_CONNECT,   /**< Waiting for the other VM to connect via serial.   */
    STATE_PLACE_SHIPS_P1,    /**< Player 1 is placing ships.                        */
    STATE_HANDOVER_P2,       /**< Transition screen asking P1 to hand over to P2.   */
    STATE_PLACE_SHIPS_P2,    /**< Player 2 is placing ships.                        */
    STATE_HANDOVER_P1,       /**< Transition screen asking P2 to hand back to P1.   */
    STATE_PLACE_SHIPS_WAITING,/**< Host is waiting for the client to finish placing. */
    STATE_COUNTDOWN,         /**< Pre-combat countdown animation.                   */
    STATE_TURN_P1,           /**< Player 1's attack turn.                           */
    STATE_TURN_P2,           /**< Player 2's attack turn.                           */
    STATE_PAUSED,            /**< Game is paused (pause menu visible).              */
    STATE_GAME_OVER,         /**< One player's fleet has been entirely sunk.        */
    STATE_EXIT               /**< Application is shutting down.                     */
} game_state_t;

/* ------------------------------------------------------------------ */
/* Main game structure                                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief The complete game state.
 *
 * The @c data union carries per-state context — only the member that
 * matches the current @c tag is valid.  Never write to this struct
 * directly; use the controller functions instead.
 */
typedef struct {
    game_state_t tag;      /**< Active state.                                  */
    game_state_t prev;     /**< Previous state (used for pause/resume).        */

    game_role_t  role;     /**< Whether this instance is host or client.       */
    bool         connected;/**< @c true once the serial handshake is complete. */

    bool         is_single_player;  
    uint8_t      bot_timer;

    board_t p1_board;      /**< Player 1's board (ships + attacks received).   */
    board_t p2_board;      /**< Player 2's board (ships + attacks received).   */

    rtc_time_t rtc;        /**< Last RTC reading (displayed in the HUD).       */
    uint32_t   tick_count; /**< Total ticks elapsed since the game started.    */
    uint32_t   timer_seconds;   /**< Total seconds elapsed (used for the HUD stopwatch). */

    uint32_t countdown_ticks;   /**< Ticks elapsed in the current countdown second. */
    uint8_t  countdown_seconds; /**< Seconds remaining on the countdown display.    */

    int8_t remote_cursor_col; /**< Column reported by the remote player's cursor. */
    int8_t remote_cursor_row; /**< Row reported by the remote player's cursor.    */

    /**
     * @brief Per-state data — only the member matching @c tag is valid.
     */
    union {
        struct { int selected; } menu;  /**< Currently highlighted menu option.       */
        struct { int selected; } pause; /**< Currently highlighted pause-menu option. */

        /** @brief State data for ship-placement phases. */
        struct {
            int player;     /**< Which player is currently placing (1 or 2).    */
            int ship_idx;   /**< Index of the ship being placed (0..NUM_SHIPS). */
            int orient;     /**< Current placement orientation.                 */
            int cursor_col; /**< Grid column under the placement cursor.        */
            int cursor_row; /**< Grid row under the placement cursor.           */
        } place;

        /** @brief State data for attack turns. */
        struct {
            int  player;     /**< Active attacking player (1 or 2).             */
            int  cursor_col; /**< Grid column the attack cursor is hovering over.*/
            int  cursor_row; /**< Grid row the attack cursor is hovering over.   */
            bool last_hit;   /**< Whether the previous attack was a hit.         */
        } turn;

        struct { int winner; } game_over; /**< Index of the winning player (1 or 2). */
    } data;

} game_t;

/* ------------------------------------------------------------------ */
/* Controller API                                                      */
/* ------------------------------------------------------------------ */

/**
 * @brief Initialises all game state.
 *
 * Must be called once before any other game function.
 *
 * @param role Whether this instance is the host or the client.
 */
void game_init(game_role_t role);

/** @brief Advances the game clock by one tick; drives animations and timeouts. */
void game_handle_timer(void);

/**
 * @brief Processes a keyboard event.
 * @param scancode Raw scancode from the KBC.
 */
void game_handle_keyboard(uint8_t scancode);

/**
 * @brief Processes a mouse event.
 * @param ms Current mouse state (position, buttons, edge flags).
 */
void game_handle_mouse(mouse_state_t *ms);

/**
 * @brief Renders the current game state to the framebuffer.
 * @param g Game state to draw (typically from @ref game_get_state).
 */
void game_draw(const game_t *g);

/** @brief Erases the cursor sprite from its last drawn position. */
void game_erase_cursor(void);

/**
 * @brief Records the cursor's current pixel position for the next erase call.
 * @param x Cursor X in pixels.
 * @param y Cursor Y in pixels.
 */
void game_save_cursor(int16_t x, int16_t y);

/** @brief Returns @c true when the game has reached @ref STATE_GAME_OVER or @ref STATE_EXIT. */
bool game_is_over(void);

/**
 * @brief Dispatches an incoming serial message to the appropriate handler.
 * @param msg Fully-decoded message from @ref proto_feed_byte.
 */
void game_handle_serial_msg(const serial_msg_t *msg);

/** @brief Returns @c true while the game is waiting for the remote VM to connect. */
bool game_is_waiting_connect(void);

/** @brief Returns @c true once the serial handshake has completed. */
bool game_is_connected(void);

/**
 * @brief Returns @c true when it is the client VM's turn to act.
 *
 * Used by the client to decide whether to forward input or process it
 * locally.
 */
bool game_is_client_turn(void);

/**
 * @brief Returns a read-only pointer to the current game state.
 *
 * The returned pointer is valid until the next call to any game function.
 * Do not store it across ticks.
 */
const game_t *game_get_state(void);
