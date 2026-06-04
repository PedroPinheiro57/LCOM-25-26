/**
 * @file protocol.h
 * @brief Serial communication protocol for the two-VM Battleship session.
 *
 * Each message consists of a single-byte type tag followed by a fixed-
 * length payload (which may be zero bytes for simple signals).  The
 * receiver assembles bytes through @ref proto_feed_byte until a complete
 * message is available.
 *
 * Message flow overview:
 *  - Host and client exchange @c MSG_HELLO / @c MSG_HELLO_ACK to establish
 *    the connection.
 *  - During ship placement the client mirrors its actions via
 *    @c MSG_SHIP_PLACE and signals completion with @c MSG_DONE_PLACING.
 *  - During combat, the active player sends @c MSG_ATTACK; the host replies
 *    with the result embedded in the same message type.
 *  - Cursor positions are streamed with @c MSG_CURSOR so each side can show
 *    where the other player is hovering.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

/** @name Message type bytes */
/** @{ */
#define MSG_HELLO          'H' /**< Client → Host: initiate handshake.               */
#define MSG_HELLO_ACK      'A' /**< Host → Client: handshake acknowledged.           */
#define MSG_KEY            'K' /**< Client → Host: forward a keyboard scancode.      */
#define MSG_MOUSE          'M' /**< Client → Host: forward a raw PS/2 mouse packet.  */
#define MSG_SHIP_PLACE     'P' /**< Client → Host: place a ship on the board.        */
#define MSG_ATTACK         'T' /**< Bidirectional: attack a cell / return result.    */
#define MSG_CURSOR         'C' /**< Bidirectional: report current cursor cell.       */
#define MSG_STATE          'S' /**< Host → Client: push a new game-state tag.        */
#define MSG_COUNTDOWN      'D' /**< Host → Client: current countdown value (seconds).*/
#define MSG_WINNER         'W' /**< Host → Client: game-over with winning player.    */
#define MSG_DONE_PLACING   'F' /**< Client → Host: all ships have been placed.       */
#define MSG_CLIENT_QUIT    'Q' /**< Client → Host: player quit the game.             */
/** @} */

/** @name Payload lengths (bytes after the type byte) */
/** @{ */
#define MSG_HELLO_LEN         0
#define MSG_HELLO_ACK_LEN     0
#define MSG_KEY_LEN           1 /**< [scancode]                               */
#define MSG_MOUSE_LEN         3 /**< [byte0, byte1, byte2] – raw PS/2 packet  */
#define MSG_SHIP_PLACE_LEN    4 /**< [col, row, size, type_idx|orient]        */
#define MSG_ATTACK_LEN        3 /**< [col, row, result]                       */
#define MSG_CURSOR_LEN        2 /**< [col, row]                               */
#define MSG_STATE_LEN         1 /**< [state]                                  */
#define MSG_COUNTDOWN_LEN     1 /**< [seconds]                                */
#define MSG_WINNER_LEN        1 /**< [winner player index]                    */
#define MSG_DONE_PLACING_LEN  0
#define MSG_CLIENT_QUIT_LEN   0
/** @} */

/** @brief Maximum payload size across all message types. */
#define MSG_MAX_PAYLOAD  4

/** @name Attack result codes (used in @c MSG_ATTACK payload) */
/** @{ */
#define ATTACK_MISS  0 /**< Shot landed in empty water.                */
#define ATTACK_HIT   1 /**< Shot hit a ship (not yet sunk).            */
#define ATTACK_SUNK  2 /**< Shot sank the last remaining cell of a ship. */
/** @} */

/**
 * @brief A fully decoded serial message ready for the game to consume.
 *
 * The active union member is determined by @c type.
 */
typedef struct {
    uint8_t type; /**< One of the @c MSG_* type constants. */

    union {
        struct { uint8_t scancode; }                    key;      /**< Valid when type == MSG_KEY.        */
        struct { uint8_t pkt[3]; }                      mouse;    /**< Valid when type == MSG_MOUSE.      */
        struct { uint8_t col, row, size, type_orient; } ship;     /**< Valid when type == MSG_SHIP_PLACE. */
        struct { uint8_t col, row, result; }            attack;   /**< Valid when type == MSG_ATTACK.     */
        struct { uint8_t col, row; }                    cursor;   /**< Valid when type == MSG_CURSOR.     */
        struct { uint8_t state; }                       state;    /**< Valid when type == MSG_STATE.      */
        struct { uint8_t seconds; }                     countdown;/**< Valid when type == MSG_COUNTDOWN.  */
        struct { uint8_t winner; }                      winner;   /**< Valid when type == MSG_WINNER.     */
    } payload;
} serial_msg_t;

/**
 * @brief Incremental receive state for the protocol parser.
 *
 * Feed incoming bytes one at a time via @ref proto_feed_byte.  When a
 * complete message has been assembled the function returns @c true and
 * populates the caller's @ref serial_msg_t.
 */
typedef struct {
    uint8_t type;       /**< Type byte received at the start of the current message. */
    uint8_t buf[MSG_MAX_PAYLOAD]; /**< Raw payload bytes accumulated so far.         */
    uint8_t collected;  /**< Number of payload bytes collected.                      */
    uint8_t expected;   /**< Number of payload bytes expected for this message type. */
    bool    have_type;  /**< @c true once the type byte has been received.           */
} proto_rx_state_t;

/**
 * @brief Returns a pointer to the global receive state singleton.
 *
 * There is normally only one UART, so a single shared state is sufficient.
 */
proto_rx_state_t *get_rx_state(void);

/**
 * @brief Resets the receive state so the parser is ready for the next message.
 * @param s Receive state to reset.
 */
void proto_rx_reset(proto_rx_state_t *s);

/**
 * @brief Feeds one byte into the protocol parser.
 *
 * Call this from the UART interrupt handler for every received byte.
 * When the function returns @c true a complete, decoded message has been
 * written to @p msg_out and the state has been reset automatically.
 *
 * @param s       Parser state (call @ref get_rx_state for the singleton).
 * @param b       Byte just received from the UART.
 * @param msg_out Filled with the decoded message when the return value is @c true.
 * @return @c true if a full message is ready, @c false if more bytes are needed.
 */
bool proto_feed_byte(proto_rx_state_t *s, uint8_t b, serial_msg_t *msg_out);

/** @name Message senders — convenience wrappers around uart_send_buf() */
/** @{ */
void proto_send_hello(void);
void proto_send_hello_ack(void);

/** @brief Sends the keyboard scancode forwarded from the client. */
void proto_send_key(uint8_t scancode);

/** @brief Forwards a raw three-byte PS/2 mouse packet. */
void proto_send_mouse(uint8_t pkt[3]);

/** @brief Notifies the host of a ship placement decision. */
void proto_send_ship_place(uint8_t col, uint8_t row, uint8_t size,
                           uint8_t type_idx, uint8_t orient);

/** @brief Sends an attack at (col, row) with its resolved result. */
void proto_send_attack(uint8_t col, uint8_t row, uint8_t result);

/** @brief Broadcasts the sender's current cursor position. */
void proto_send_cursor(uint8_t col, uint8_t row);

/** @brief Pushes the current game-state tag to the remote side. */
void proto_send_state(uint8_t state);

/** @brief Pushes the current countdown value (seconds remaining). */
void proto_send_countdown(uint8_t seconds);

/** @brief Announces which player has won the game. */
void proto_send_winner(uint8_t winner);

/** @brief Signals that all ships have been placed; waiting for the opponent. */
void proto_send_done_placing(void);

/** @brief Signals that the client player has exited the session. */
void proto_send_client_quit(void);
/** @} */
