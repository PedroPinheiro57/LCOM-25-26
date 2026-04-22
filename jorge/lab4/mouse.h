#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <lcom/lcf.h>

/* Mouse IRQ line */
#define MOUSE_IRQ       12

/* Mouse commands (sent via KBC 0xD4 → 0x60) */
#define MOUSE_ENABLE_DR   0xF4   /* Enable Data Reporting  */
#define MOUSE_DISABLE_DR  0xF5   /* Disable Data Reporting */
#define KBC_WRITE_MOUSE   0xD4   /* KBC cmd: forward next byte to mouse */

/* Mouse acknowledgement bytes */
#define MOUSE_ACK         0xFA   /* Success */
#define MOUSE_NACK        0xFE   /* Resend  */
#define MOUSE_ERROR       0xFC   /* Error   */

/* PS/2 packet byte 1 bits */
#define MOUSE_LB          BIT(0)  /* Left button   */
#define MOUSE_RB          BIT(1)  /* Right button  */
#define MOUSE_MB          BIT(2)  /* Middle button */
#define MOUSE_SYNC_BIT    BIT(3)  /* Always 1 in byte 1 - used for sync */
#define MOUSE_X_SIGN      BIT(4)  /* X movement sign bit */
#define MOUSE_Y_SIGN      BIT(5)  /* Y movement sign bit */
#define MOUSE_X_OVF       BIT(6)  /* X overflow */
#define MOUSE_Y_OVF       BIT(7)  /* Y overflow */

/* Subscribe/unsubscribe mouse interrupts */
int mouse_subscribe_int(uint8_t *bit_no);
int mouse_unsubscribe_int(void);

/* Interrupt handler - reads one byte from KBC output buffer */
void (mouse_ih)(void);

/* Send a command byte to the mouse (handles 0xD4 + write + ACK check) */
int mouse_send_cmd(uint8_t cmd);

/* Disable data reporting (enable uses the LCF-provided macro) */
int mouse_disable_data_reporting(void);

/* Getters for data set by mouse_ih() */
uint8_t mouse_get_byte(void);
bool    mouse_has_error(void);
