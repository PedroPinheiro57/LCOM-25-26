/**
 * @file uart.h
 * @brief 16550-compatible UART driver for COM1 serial communication.
 *
 * Configures the UART at 9600 baud, 8-N-1, with FIFOs enabled and a
 * 4-byte RX trigger threshold (matching the maximum protocol payload
 * size).  Received bytes are stored in a software ring buffer and
 * consumed by @ref uart_recv_byte.
 *
 * Typical usage:
 * @code
 *   uint8_t irq_bit;
 *   uart_init(&irq_bit);
 *   // ... subscribe to IRQ4, point handler to uart_ih() ...
 *   uart_send_byte(0x48);   // 'H'
 *   uint8_t b;
 *   if (uart_recv_byte(&b)) { // process b }
 *   uart_cleanup();
 * @endcode
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <lcom/lcf.h>

/** @name Hardware base address and IRQ */
/** @{ */
#define UART_COM1_BASE  0x3F8 /**< I/O base address for COM1. */
#define UART_COM1_IRQ   4     /**< IRQ line used by COM1.     */
/** @} */

/**
 * @name Register offsets (DLAB = 0)
 * Add these to @ref UART_COM1_BASE to get the actual port address.
 * @{
 */
#define UART_RBR  0 /**< Receiver Buffer Register        (read-only).       */
#define UART_THR  0 /**< Transmitter Holding Register    (write-only).      */
#define UART_IER  1 /**< Interrupt Enable Register       (read/write).      */
#define UART_IIR  2 /**< Interrupt Identification Reg.   (read-only).       */
#define UART_FCR  2 /**< FIFO Control Register           (write-only).      */
#define UART_LCR  3 /**< Line Control Register           (read/write).      */
#define UART_MCR  4 /**< Modem Control Register          (read/write).      */
#define UART_LSR  5 /**< Line Status Register            (read-only).       */
/** @} */

/**
 * @name Register offsets (DLAB = 1)
 * Set @ref UART_LCR_DLAB before accessing these.
 * @{
 */
#define UART_DLL  0 /**< Divisor Latch – least significant byte. */
#define UART_DLM  1 /**< Divisor Latch – most significant byte.  */
/** @} */

/**
 * @name LCR – Line Control Register bits
 * @{
 */
#define UART_LCR_WLS_5  0                  /**< 5 data bits.                     */
#define UART_LCR_WLS_6  BIT(0)             /**< 6 data bits.                     */
#define UART_LCR_WLS_7  BIT(1)             /**< 7 data bits.                     */
#define UART_LCR_WLS_8  (BIT(1) | BIT(0)) /**< 8 data bits (default).           */
#define UART_LCR_DLAB   BIT(7)            /**< Divisor Latch Access Bit.         */
/** @} */

/**
 * @name IER – Interrupt Enable Register bits
 * @{
 */
#define UART_IER_RDI     BIT(0) /**< Enable interrupt on received data available. */
#define UART_IER_RLSI    BIT(2) /**< Enable interrupt on receiver line status.    */
#define UART_IER_DISABLE 0x00   /**< Mask to disable all UART interrupts.         */
/** @} */

/**
 * @name IIR – Interrupt Identification Register bits
 * @{
 */
#define UART_IIR_NO_INT  BIT(0)                   /**< Set when no interrupt is pending.       */
#define UART_IIR_ID_MASK (BIT(3)|BIT(2)|BIT(1))   /**< Mask for the interrupt type field.      */
#define UART_IIR_RLS     (BIT(2)|BIT(1))           /**< Receiver Line Status  (highest priority). */
#define UART_IIR_RDA     BIT(2)                    /**< Received Data Available.                */
#define UART_IIR_CTI     (BIT(3)|BIT(2))           /**< Character Timeout Indication.           */
#define UART_IIR_FIFO_EN (BIT(7)|BIT(6))           /**< Both bits set when FIFOs are enabled.   */
/** @} */

/**
 * @name FCR – FIFO Control Register bits
 * @{
 */
#define UART_FCR_ENABLE  BIT(0)            /**< Enable RX and TX FIFOs.                     */
#define UART_FCR_RXRST   BIT(1)            /**< Reset RX FIFO (self-clearing).              */
#define UART_FCR_TXRST   BIT(2)            /**< Reset TX FIFO (self-clearing).              */
#define UART_FCR_TRIG_1  0                 /**< RX interrupt trigger level:  1 byte.        */
#define UART_FCR_TRIG_4  BIT(6)            /**< RX interrupt trigger level:  4 bytes.       */
#define UART_FCR_TRIG_8  BIT(7)            /**< RX interrupt trigger level:  8 bytes.       */
#define UART_FCR_TRIG_14 (BIT(7)|BIT(6))   /**< RX interrupt trigger level: 14 bytes.       */
/** @brief Default FCR value: FIFOs on, both reset, 4-byte trigger threshold. */
#define UART_FCR_INIT  (UART_FCR_ENABLE | UART_FCR_RXRST | UART_FCR_TXRST | UART_FCR_TRIG_4)
/** @} */


/**
 * @name LSR – Line Status Register bits
 * @{
 */
#define UART_LSR_DR      BIT(0) /**< Data ready in RBR or RX FIFO.                 */
#define UART_LSR_OE      BIT(1) /**< Overrun error.                                */
#define UART_LSR_PE      BIT(2) /**< Parity error.                                 */
#define UART_LSR_FE      BIT(3) /**< Framing error.                                */
#define UART_LSR_THRE    BIT(5) /**< TX Holding Register empty – safe to write.    */
#define UART_LSR_FIFOERR BIT(7) /**< At least one error in the RX FIFO.            */
/** @brief Mask covering all three receivable error bits. */
#define UART_LSR_ERR_MASK (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE)
/** @} */

/**
 * @name Baud rate divisors
 * Divisor = 115200 / desired_baud.
 * @{
 */
#define UART_BAUD_115200     1  /**< Divisor for 115200 baud (maximum speed) */

#define UART_DEFAULT_BAUD    UART_BAUD_115200
/** @} */

/** @brief Default line format: 8 data bits, no parity, 1 stop bit. */
#define UART_DEFAULT_LCR  UART_LCR_WLS_8

/** @brief Capacity of the software RX ring buffer (bytes). */
#define UART_RXBUF_SIZE  128

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

/**
 * @brief Initialises the UART hardware and software ring buffer.
 *
 * Programs the baud rate divisor, line format, FIFOs, and interrupts,
 * then subscribes the IRQ.
 *
 * @param bit_no Output: the IRQ bit position assigned by the LCF layer,
 *               needed to call @c lcf_irq_subscribe.
 * @return 0 on success, non-zero on failure.
 */
int  uart_init(uint8_t *bit_no);

/**
 * @brief Disables UART interrupts and releases the IRQ subscription.
 */
void uart_cleanup(void);

/**
 * @brief UART interrupt handler — must be called from the IRQ dispatcher.
 *
 * Reads all available bytes from the hardware FIFO into the software
 * ring buffer and handles RLS/CTI interrupt causes.
 */
void uart_ih(void);

/**
 * @brief Sends a single byte, blocking until the TX holding register is empty.
 *
 * @param b Byte to transmit.
 * @return 0 on success, non-zero if the LSR could not be read.
 */
int  uart_send_byte(uint8_t b);

/**
 * @brief Sends a buffer of bytes sequentially via @ref uart_send_byte.
 *
 * @param buf Pointer to the data to send.
 * @param len Number of bytes to send.
 * @return 0 on success, non-zero on the first send failure.
 */
int  uart_send_buf(const uint8_t *buf, uint8_t len);

/**
 * @brief Reads one byte from the software RX ring buffer.
 *
 * Non-blocking: returns immediately if the buffer is empty.
 *
 * @param b Pointer to store the received byte.
 * @return @c true if a byte was available and written to @p b.
 */
bool uart_recv_byte(uint8_t *b);

/**
 * @brief Returns @c true if the RX ring buffer has overflowed since the
 *        last call to @ref uart_init (bytes were silently dropped).
 */
bool uart_rx_overflow(void);
