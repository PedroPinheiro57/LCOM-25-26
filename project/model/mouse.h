/**
 * @file mouse.h
 * @brief PS/2 mouse packet parsing and cursor-state tracking.
 *
 * Decodes the three-byte PS/2 mouse packets into a high-level
 * @ref mouse_state_t that the rest of the game reads each tick.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Bitfield overlay for the first byte of a PS/2 mouse packet.
 *
 * Lets you access button flags and delta sign/overflow bits by name
 * rather than masking the raw byte manually.
 */
typedef union {
    uint8_t val; /**< Raw first byte of the PS/2 packet. */

    struct {
        uint8_t lb     : 1; /**< Left button pressed.                  */
        uint8_t rb     : 1; /**< Right button pressed.                 */
        uint8_t mb     : 1; /**< Middle button pressed.                */
        uint8_t sync   : 1; /**< Sync bit – always 1 on a valid packet.*/
        uint8_t x_sign : 1; /**< Sign bit for the X displacement.      */
        uint8_t y_sign : 1; /**< Sign bit for the Y displacement.      */
        uint8_t x_ovf  : 1; /**< X displacement overflowed.            */
        uint8_t y_ovf  : 1; /**< Y displacement overflowed.            */
    } fields;
} mouse_byte1_t;

/**
 * @brief Aggregated mouse state, updated every time a full packet arrives.
 *
 * The @c clicked and @c released fields are edge-triggered: they are only
 * @c true for the single packet in which the left button transitions.
 */
typedef struct {
    int16_t x, y;     /**< Current cursor position in screen pixels.          */
    bool    lb;       /**< Left button is currently held down.                 */
    bool    rb;       /**< Right button is currently held down.                */
    bool    mb;       /**< Middle button is currently held down.               */
    bool    clicked;  /**< Left button went down this packet (rising edge).    */
    bool    released; /**< Left button went up this packet (falling edge).     */
    bool    moved;    /**< Cursor position changed this packet.                */
} mouse_state_t;

/**
 * @brief Initialises a mouse state struct and places the cursor at the
 *        given screen coordinates.
 *
 * @param ms      State struct to initialise.
 * @param start_x Initial X position.
 * @param start_y Initial Y position.
 */
void mouse_state_init(mouse_state_t *ms, int16_t start_x, int16_t start_y);

/**
 * @brief Decodes a raw three-byte PS/2 packet and updates the mouse state.
 *
 * Clamps the resulting position to the screen boundaries defined by
 * @p hres and @p vres.
 *
 * @param ms   Mouse state to update.
 * @param buf  Three-byte PS/2 packet (bytes 0–2).
 * @param hres Screen width  used for X clamping.
 * @param vres Screen height used for Y clamping.
 */
void mouse_state_update(mouse_state_t *ms, uint8_t buf[3],
                        uint16_t hres, uint16_t vres);

/** @brief Returns a pointer to the global mouse state. */
mouse_state_t *get_mouse_state(void);

/** @brief Returns a pointer to the three-byte PS/2 receive buffer. */
uint8_t       *get_mouse_buf(void);

/** @brief Returns the current write index into the PS/2 receive buffer. */
uint8_t        get_mouse_idx(void);

/**
 * @brief Returns @c true when a complete three-byte packet has been
 *        assembled and is ready to be decoded.
 */
bool           get_mouse_packet_ready(void);

/** @brief Sets the write index into the PS/2 receive buffer. */
void           set_mouse_idx(uint8_t val);

/** @brief Marks whether a full PS/2 packet is waiting to be processed. */
void           set_mouse_packet_ready(bool val);
