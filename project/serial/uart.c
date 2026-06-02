#include "uart.h"
#include "protocol.h"
#include <lcom/lcf.h>

static int uart_hook_id = UART_COM1_IRQ;  

/* Software RX ring buffer */
#define RXBUF_MASK  (UART_RXBUF_SIZE - 1)

static uint8_t rxbuf_data[UART_RXBUF_SIZE];
static uint8_t rxbuf_head = 0;
static uint8_t rxbuf_tail = 0;

static void rxbuf_push(uint8_t b) {
    uint8_t next = (rxbuf_head + 1) & RXBUF_MASK;
    if (next != rxbuf_tail) {
        rxbuf_data[rxbuf_head] = b;
        rxbuf_head = next;
    }
}

static bool rxbuf_pop(uint8_t *out) {
    if (rxbuf_tail == rxbuf_head) return false;
    *out = rxbuf_data[rxbuf_tail];
    rxbuf_tail = (rxbuf_tail + 1) & RXBUF_MASK;
    return true;
}

/* Low-level register helpers */
static uint8_t uart_read_reg(uint8_t offset) {
    uint32_t val = 0;
    sys_inb(UART_COM1_BASE + offset, &val);
    return (uint8_t)(val & 0xFF);
}

static void uart_write_reg(uint8_t offset, uint8_t val) {
    sys_outb(UART_COM1_BASE + offset, val);
}

int uart_init(uint8_t *bit_no) {
    proto_rx_reset(get_rx_state());

    if (bit_no == NULL) return 1;

    // Set DLAB = 1 to expose the Baud Rate Divisor Latches (DLL and DLM)
    uint8_t lcr_dlab = UART_LCR_DLAB | UART_DEFAULT_LCR;
    if (sys_outb(UART_COM1_BASE + UART_LCR, lcr_dlab) != 0) return 1;
    
    // Write the 9600 baud rate divisors (12 and 0)
    if (sys_outb(UART_COM1_BASE + UART_DLL, UART_DEFAULT_BAUD) != 0) return 1;
    if (sys_outb(UART_COM1_BASE + UART_DLM, 0) != 0) return 1;

    // Reset DLAB to 0 so Offset 1 maps back to the IER register
    if (sys_outb(UART_COM1_BASE + UART_LCR, UART_DEFAULT_LCR) != 0) return 1;

    // Turn on Received Data Interrupts (RDI) in the IER register
    if (sys_outb(UART_COM1_BASE + UART_IER, UART_IER_RDI) != 0) return 1;

    // Enable OUT2 in the Modem Control Register (MCR)
    // Offset 4 is the MCR. Bit 3 (0x08) is OUT2. 
    uint8_t mcr_val = 0x08; 
    if (sys_outb(UART_COM1_BASE + 4, mcr_val) != 0) return 1;

    // FORCE RESETS THE HARDWARE LINE: Clear out any junk left from previous crashes.
    // If bytes from a prior crash are still here, the IRQ line is locked HIGH, 
    // preventing the Host from ever detecting a new edge-triggered interrupt.
    uint8_t lsr_check = 0;
    // Read the current Line Status Register (LSR is offset 5)
    uint32_t val_lsr = 0;
    sys_inb(UART_COM1_BASE + 5, &val_lsr);
    lsr_check = (uint8_t)(val_lsr & 0xFF);

    while (lsr_check & UART_LSR_DR) { // As long as Data Ready (Bit 0) is true
        uint32_t dummy_rbr = 0;
        sys_inb(UART_COM1_BASE + 0, &dummy_rbr); // Read RBR to discard the ghost byte
        
        // Refresh LSR to see if more bytes remain
        sys_inb(UART_COM1_BASE + 5, &val_lsr);
        lsr_check = (uint8_t)(val_lsr & 0xFF);
    }

    // 6. Expose the expected interrupt bit position to main.c before the kernel alters it.
    // Since uart_hook_id starts as 4, main.c will look for BIT(4) (0x10) in its loop mask.
    //*bit_no = (uint8_t) uart_hook_id;

    // Subscribe to the kernel IRQ policy
    if (sys_irqsetpolicy(UART_COM1_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &uart_hook_id) != 0) {
        return 1;
    }

    return 0;
}

/* uart_cleanup */
void uart_cleanup(void) {
    // Turn off UART interrupts completely on the hardware side
    sys_outb(UART_COM1_BASE + UART_IER, 0);
    
    // Clear OUT2 on the Modem Control Register to completely disconnect lines
    sys_outb(UART_COM1_BASE + 4, 0);
    
    sys_irqrmpolicy(&uart_hook_id);
}

/* uart_ih */
void uart_ih(void) {
    // 1. Read the Interrupt Identification Register (IIR)
    uint8_t iir = uart_read_reg(UART_IIR);
    
    // Bit 0 of IIR is the "Interrupt Status" bit. 
    // If it is 1 (UART_IIR_NO_INT), it means this specific hardware chip 
    // did NOT generate an interrupt. Exit immediately
    if (iir & UART_IIR_NO_INT) {
        return; 
    }

    // 2. Read the Line Status Register (LSR) to see what happened
    uint8_t lsr = uart_read_reg(UART_LSR);

    // 3. Drain the hardware FIFO/Registers into your software ring buffer
    // As long as Bit 0 (UART_LSR_DR - Data Ready) is 1, keep pulling bytes!
    while (lsr & UART_LSR_DR) {
        uint8_t b = uart_read_reg(UART_RBR);
        
        rxbuf_push(b);
        
        // Refresh the LSR to verify if another byte arrived while processing
        lsr = uart_read_reg(UART_LSR);
    }
}

/* uart_send_byte — polled TX */
int uart_send_byte(uint8_t b) {
    uint8_t lsr;
    int attempts = 10000000;
    while (attempts--) {
        lsr = uart_read_reg(UART_LSR);
        if (lsr & UART_LSR_THRE) {
            uart_write_reg(UART_THR, b);
            return 0;
        }
    }
    return 1;
}

/* uart_recv_byte */
bool uart_recv_byte(uint8_t *b) {
    return rxbuf_pop(b);
}

/* uart_rx_available */
uint8_t uart_rx_available(void) {
    return (uint8_t)((rxbuf_head - rxbuf_tail) & RXBUF_MASK);
}
