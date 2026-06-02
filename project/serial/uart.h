#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <lcom/lcf.h>


#define UART_COM1_BASE   0x3F8
#define UART_COM1_IRQ    4

/* Register addresses (DLAB=0) */
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

/* Register addresses (DLAB=1) */
#define UART_DLL   0   /* Divisor Latch LSB               (RW)        */
#define UART_DLM   1   /* Divisor Latch MSB               (RW)        */

/* LCR bits                                                           */
#define UART_LCR_WLS_8    0x03 /* nº of bits per char. 00->5, 01->6, 10->7, 11->8 */
#define UART_LCR_DLAB     BIT(7) /* divisor latch access */

/* IER bits                                                           */
#define UART_IER_RDI      BIT(0)   /* Received Data Available       */
#define UART_IER_DISABLE  0x00     /* disable every bit */

/* IIR bits                                                           */
#define UART_IIR_NO_INT   BIT(0)     /* No interrupt pending (active-low) */
#define UART_IIR_ID_MASK  0x0E       /* if bits 1, 2 or 3 are active */
#define UART_IIR_RLS      0x06       /* Receiver Line Status           */
#define UART_IIR_RDA      0x04       /* Received Data Available        */
#define UART_IIR_CTI      0x0C       /* Character Timeout              */
#define UART_IIR_THRE     0x02       /* THR Empty                      */

/* FCR bits                                                           */
#define UART_FCR_ENABLE   BIT(0) /* enable both fifos */
#define UART_FCR_RXRST    BIT(1) /* clear all bytes in RCVR FIFO. Self-clearing */
#define UART_FCR_TXRST    BIT(2) /* clear all bytes in the XMIT FIFO. Self-clearing */
#define UART_FCR_TRIG_1   0x00   /* trigger level - bits 6 e 7 - 00->1, 01->4, 10->8, 11->14*/

/* LSR bits                                                         */
#define UART_LSR_DR       BIT(0)   /* Data Ready                    */
#define UART_LSR_OE       BIT(1)   /* Overrun Error                 */
#define UART_LSR_PE       BIT(2)   /* Parity Error                  */
#define UART_LSR_FE       BIT(3)   /* Framing Error                 */
#define UART_LSR_THRE     BIT(5)   /* TX Holding Register Empty     */
#define UART_LSR_ERR_MASK (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE) /* any of the errors above */

/* Baud rate                                                          */
#define UART_BAUD_9600       12
#define UART_DEFAULT_BAUD    UART_BAUD_9600
#define UART_DEFAULT_LCR     UART_LCR_WLS_8   /* 8-N-1                */

/* RX software ring buffer                                            */
#define UART_RXBUF_SIZE  64


int     uart_init(uint8_t *hook_id);
void    uart_cleanup(void);
void    uart_ih(void);
int     uart_send_byte(uint8_t b);
bool    uart_recv_byte(uint8_t *b);
uint8_t uart_rx_available(void);
