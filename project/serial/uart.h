#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <lcom/lcf.h>

/*  Hardware constants */
#define UART_COM1_BASE   0x3F8
#define UART_COM1_IRQ    4

/*  Register offsets  (DLAB = 0) */
#define UART_RBR   0   /* Receiver Buffer Register        (R)          */
#define UART_THR   0   /* Transmitter Holding Register    (W)          */
#define UART_IER   1   /* Interrupt Enable Register       (RW)         */
#define UART_IIR   2   /* Interrupt Identification Reg.   (R)          */
#define UART_FCR   2   /* FIFO Control Register           (W)          */
#define UART_LCR   3   /* Line Control Register           (RW)         */
#define UART_MCR   4   /* Modem Control Register          (RW)         */
#define UART_LSR   5   /* Line Status Register            (R)          */

/* Register offsets  (DLAB = 1) */
#define UART_DLL   0   /* Divisor Latch LSB               (RW)         */
#define UART_DLM   1   /* Divisor Latch MSB               (RW)         */

/* LCR – Line Control Register */
#define UART_LCR_WLS_5    0                /* bits [1:0] = 00 → 5 bits   */
#define UART_LCR_WLS_6    BIT(0)           /* bits [1:0] = 01 → 6 bits   */
#define UART_LCR_WLS_7    BIT(1)           /* bits [1:0] = 10 → 7 bits   */
#define UART_LCR_WLS_8    (BIT(1) | BIT(0))/* bits [1:0] = 11 → 8 bits   */
#define UART_LCR_DLAB     BIT(7) /* Divisor Latch Access Bit           */

/*  IER  –  Interrupt Enable Register */
#define UART_IER_RDI      BIT(0) /* Received Data Available interrupt  */
#define UART_IER_RLSI     BIT(2) /* Receiver Line Status interrupt     */
#define UART_IER_DISABLE  0x00   /* Disable all interrupts             */

/*  IIR  –  Interrupt Identification Register */
#define UART_IIR_NO_INT   BIT(0) /* 1 = no interrupt pending           */
#define UART_IIR_ID_MASK  (BIT(3) | BIT(2) | BIT(1))  /* bits [3:1] encode interrupt type   */
#define UART_IIR_RLS      (BIT(2) | BIT(1))           /* bits [3:1] = 011 → RLS  (priority 1)   */
#define UART_IIR_RDA      BIT(2)                      /* bits [3:1] = 010 → RDA  (prio 2)   */
#define UART_IIR_CTI      (BIT(3) | BIT(2))           /* bits [3:1] = 110 → CTI  (prio 2)   */
#define UART_IIR_FIFO_EN  (BIT(7) | BIT(6))           /* bits [7:6] = 11 when FIFOs enabled */

/*  FCR  –  FIFO Control Register */
#define UART_FCR_ENABLE   BIT(0) /* Enable both RX and TX FIFOs        */
#define UART_FCR_RXRST    BIT(1) /* Reset RX FIFO counter (self-clear) */
#define UART_FCR_TXRST    BIT(2) /* Reset TX FIFO counter (self-clear) */
#define UART_FCR_TRIG_1   0              /* bits [7:6] = 00 →  1 byte  */
#define UART_FCR_TRIG_4   BIT(6)         /* bits [7:6] = 01 →  4 bytes (we use this because max lenght of our payloads is 4 bytes)*/
#define UART_FCR_TRIG_8   BIT(7)           /* bits [7:6] = 10 →  8 bytes */
#define UART_FCR_TRIG_14  (BIT(7) | BIT(6))/* bits [7:6] = 11 → 14 bytes */
#define UART_FCR_INIT  (UART_FCR_ENABLE | UART_FCR_RXRST | UART_FCR_TXRST  | UART_FCR_TRIG_4)

/*  MCR  –  Modem Control Register */
#define UART_MCR_OUT2     BIT(3) /* OUT2 – gates IRQ through PIC on PC */
#define UART_MCR_INIT  UART_MCR_OUT2

/*  LSR  –  Line Status Register */
#define UART_LSR_DR       BIT(0) /* Data Ready in RBR or RX FIFO       */
#define UART_LSR_OE       BIT(1) /* Overrun Error                      */
#define UART_LSR_PE       BIT(2) /* Parity Error                       */
#define UART_LSR_FE       BIT(3) /* Framing Error                      */
#define UART_LSR_THRE     BIT(5) /* TX Holding Register Empty          */
#define UART_LSR_FIFOERR  BIT(7) /* ≥1 error in RX FIFO (FIFO mode)   */
#define UART_LSR_ERR_MASK (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE)

/*  Baud rate divisors */
/*  Formula: divisor = freq input / (baud_rate × 16) */
#define UART_BAUD_9600    12
#define UART_DEFAULT_BAUD  UART_BAUD_9600

/*  Default line format: 8 data bits, no parity, 1 stop bit  (8-N-1)  */
#define UART_DEFAULT_LCR   UART_LCR_WLS_8

/* Software RX ring buffer */
#define UART_RXBUF_SIZE   128


/* Public API */
int     uart_init(uint8_t *bit_no);
void    uart_cleanup(void);
void    uart_ih(void);
int     uart_send_byte(uint8_t b);
int     uart_send_buf(const uint8_t *buf, uint8_t len);
bool    uart_recv_byte(uint8_t *b);
bool    uart_rx_overflow(void);
