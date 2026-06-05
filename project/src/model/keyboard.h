/**
 * @file keyboard.h
 * @brief KBC (Keyboard Controller) register definitions and status helpers.
 *
 * Provides a bitfield overlay for the KBC status register so individual
 * flags can be read without manual bit-masking.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Overlay for the KBC status register (port 0x64).
 *
 * Read the register into @c val and then access individual flags through
 * the @c fields member.
 */
typedef union {
    uint8_t val; /**< Raw register byte. */

    /** Named bit-fields matching the KBC status register layout. */
    struct kbc_status_fields {
        uint8_t obf     : 1; /**< Output buffer full – data ready to be read. */
        uint8_t ibf     : 1; /**< Input buffer full  – controller still busy.  */
        uint8_t sys     : 1; /**< System flag (POST result).                   */
        uint8_t cmd     : 1; /**< 1 = data in input buffer is a command.       */
        uint8_t _unused : 1; /**< Reserved.                                    */
        uint8_t aux     : 1; /**< Output buffer data came from the mouse.      */
        uint8_t timeout : 1; /**< Timeout error on last transfer.              */
        uint8_t parity  : 1; /**< Parity error on last transfer.               */
    } fields;
} kbc_status_t;
