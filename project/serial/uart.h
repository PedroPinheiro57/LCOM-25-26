#pragma once
/*
 * uart.h — UART driver for COM1 (16550-compatible).
 */

#include <stdint.h>
#include <stdbool.h>
#include <lcom/lcf.h>

/* ------------------------------------------------------------------ */
/* I/O base and IRQ                                                   */
/* ------------------------------------------------------------------ */
#define UART_COM1_BASE   0x3F8
#define UART_COM1_IRQ    4

/* ------------------------------------------------------------------ */
/* Register offsets (DLAB=0)                                          */
/* ------------------------------------------------------------------ */
#define UART_RBR   0   /* Receiver Buffer Register        (R)         */
#define UART_THR   0   /* Transmitter Holding Register    (W)         */
#define UART_IER   1   /* Interrupt Enable Register       (RW)        */
#define UART_IIR   2   /* Interrupt Identification Reg.   (R)         */
#define UART_FCR   2   /* FIFO Control Register           (W)         */
#define UART_LCR   3   /* Line Control Register           (RW)        */
#define UART_MCR   4   /* Modem Control Register          (RW)        */
#define UART_LSR   5   /* Line Status Register            (R)         */
#define UART_MSR   6   /* Modem Status Register           (R)         */
#define UART_SR    7   /* Scratchpad Register             (RW)        */

/* Register offsets (DLAB=1) */
#define UART_DLL   0   /* Divisor Latch LSB               (RW)        */
#define UART_DLM   1   /* Divisor Latch MSB               (RW)        */

/* ------------------------------------------------------------------ */
/* LCR bits                                                           */
/* ------------------------------------------------------------------ */
#define UART_LCR_WLS_8    0x03
#define UART_LCR_DLAB     (1 << 7)

/* ------------------------------------------------------------------ */
/* IER bits                                                           */
/* ------------------------------------------------------------------ */
#define UART_IER_RDI      (1 << 0)   /* Received Data Available       */
#define UART_IER_DISABLE  0x00

/* ------------------------------------------------------------------ */
/* IIR bits                                                           */
/* ------------------------------------------------------------------ */
#define UART_IIR_NO_INT   (1 << 0)   /* No interrupt pending (active-low) */
#define UART_IIR_ID_MASK  0x0E
#define UART_IIR_RLS      0x06       /* Receiver Line Status           */
#define UART_IIR_RDA      0x04       /* Received Data Available        */
#define UART_IIR_CTI      0x0C       /* Character Timeout              */
#define UART_IIR_THRE     0x02       /* THR Empty                      */

/* ------------------------------------------------------------------ */
/* FCR bits                                                           */
/* ------------------------------------------------------------------ */
#define UART_FCR_ENABLE   (1 << 0)
#define UART_FCR_RXRST    (1 << 1)
#define UART_FCR_TXRST    (1 << 2)
#define UART_FCR_TRIG_1   0x00

/* ------------------------------------------------------------------ */
/* LSR bits                                                           */
/* ------------------------------------------------------------------ */
#define UART_LSR_DR       (1 << 0)   /* Data Ready                    */
#define UART_LSR_OE       (1 << 1)   /* Overrun Error                 */
#define UART_LSR_PE       (1 << 2)   /* Parity Error                  */
#define UART_LSR_FE       (1 << 3)   /* Framing Error                 */
#define UART_LSR_THRE     (1 << 5)   /* TX Holding Register Empty     */
#define UART_LSR_ERR_MASK (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE)

/* ------------------------------------------------------------------ */
/* Baud rate                                                          */
/* ------------------------------------------------------------------ */
#define UART_BAUD_9600       12
#define UART_DEFAULT_BAUD    UART_BAUD_9600
#define UART_DEFAULT_LCR     UART_LCR_WLS_8   /* 8-N-1                */

/* ------------------------------------------------------------------ */
/* RX software ring buffer                                            */
/* ------------------------------------------------------------------ */
#define UART_RXBUF_SIZE  64

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */
int     uart_init(uint8_t *hook_id);
void    uart_cleanup(void);
void    uart_ih(void);
int     uart_send_byte(uint8_t b);
bool    uart_recv_byte(uint8_t *b);
uint8_t uart_rx_available(void);
