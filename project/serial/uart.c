#include "uart.h"
#include <lcom/lcf.h>

int uart_hook_id = UART_COM1_IRQ;

/*  Software RX (receive/receiver) ring buffer */
#define RXBUF_MASK  (UART_RXBUF_SIZE - 1)

static uint8_t rxbuf_data[UART_RXBUF_SIZE];
static uint8_t rxbuf_head = 0;          /* next write index            */
static uint8_t rxbuf_tail = 0;          /* next read  index            */
static bool    rxbuf_overflowed = false; /* set on drop; cleared on read */

/* rxbuf_discard_all – Reset head to tail, throwing away every byte currently in the buffer*/
static void rxbuf_discard_all(void) {
    rxbuf_head = rxbuf_tail;
}

static void rxbuf_push(uint8_t b) {
    uint8_t next = (rxbuf_head + 1) & RXBUF_MASK;
    if (next != rxbuf_tail) {
        rxbuf_data[rxbuf_head] = b;
        rxbuf_head = next;
    } else {
        rxbuf_overflowed = true;
    }
}

static bool rxbuf_pop(uint8_t *out) {
    if (rxbuf_tail == rxbuf_head) return false;
    *out = rxbuf_data[rxbuf_tail];
    rxbuf_tail = (rxbuf_tail + 1) & RXBUF_MASK;
    return true;
}


static uint8_t uart_read_reg(uint8_t offset) {
    uint8_t val = 0;
    util_sys_inb(UART_COM1_BASE + offset, &val);
    return val;
}

static void uart_write_reg(uint8_t offset, uint8_t val) {
    sys_outb(UART_COM1_BASE + offset, val);
}

static void uart_set_baud_rate(void) {
    uart_write_reg(UART_LCR, UART_LCR_DLAB | UART_DEFAULT_LCR);
    uart_write_reg(UART_DLL, (uint8_t)(UART_DEFAULT_BAUD));
    uart_write_reg(UART_DLM, (uint8_t)(UART_DEFAULT_BAUD >> 8));
}

static void uart_set_line_control(void) {
    uart_write_reg(UART_LCR, UART_DEFAULT_LCR);
}

static void uart_configure_fifo(void) {
    uart_write_reg(UART_FCR, UART_FCR_INIT);

    if ((uart_read_reg(UART_IIR) & UART_IIR_FIFO_EN) != UART_IIR_FIFO_EN) {
        uart_write_reg(UART_FCR, 0x00); /* disable; chip lacks FIFO support */
    }
}

static void uart_enable_out2(void) {
    uart_write_reg(UART_MCR, UART_MCR_INIT);
}

static void uart_flush_rx(void) {
    uint8_t lsr = uart_read_reg(UART_LSR);

    while (lsr & UART_LSR_DR) {
        (void)uart_read_reg(UART_RBR);
        lsr = uart_read_reg(UART_LSR);
    }

    (void)uart_read_reg(UART_IIR); /* clear any latched interrupt */
}

static void uart_enable_interrupts(void) {
    uart_write_reg(UART_IER, UART_IER_RDI | UART_IER_RLSI);
}

static int uart_subscribe_irq(uint8_t *bit_no) {
    if (sys_irqsetpolicy(UART_COM1_IRQ,IRQ_REENABLE | IRQ_EXCLUSIVE,&uart_hook_id) != 0) return 1;
    return 0;
}

int uart_init(uint8_t *bit_no) {
    if (bit_no == NULL) return 1;

    uart_set_baud_rate();       
    uart_set_line_control();   
    uart_configure_fifo();     
    uart_enable_out2();        
    uart_flush_rx();            
    uart_enable_interrupts(); 

    return uart_subscribe_irq(bit_no); 
}


void uart_cleanup(void) {
    uart_write_reg(UART_IER, UART_IER_DISABLE);
    uart_write_reg(UART_MCR, 0x00);
    uart_write_reg(UART_FCR, 0x00);
    sys_irqrmpolicy(&uart_hook_id);
}


void uart_ih(void) {
    uint8_t iir = uart_read_reg(UART_IIR);

    if (iir & UART_IIR_NO_INT) return;

    uint8_t int_id = iir & UART_IIR_ID_MASK;

    /* Case 1: Receiver Line Status (priority 1) */
    if (int_id == UART_IIR_RLS) {
        uint8_t lsr = uart_read_reg(UART_LSR); /* clears the RLS interrupt */

        if (lsr & UART_LSR_ERR_MASK) {
            /*
             * Discard bytes already in the ring buffer — they may
             * belong to the same message as the corrupted byte.
             */
            rxbuf_discard_all();
            rxbuf_overflowed = true;
        }

        /* Drain FIFO to deassert IRQ, discarding every byte
         * because we cannot trust any of them after a line error. */
        while (lsr & UART_LSR_DR) {
            uart_read_reg(UART_RBR);
            lsr = uart_read_reg(UART_LSR);
        }
        return;
    }

    /* Case 2: Received Data Available (RDA) or Character Timeout */
    /* Drain the entire RX FIFO */
    if (int_id == UART_IIR_RDA || int_id == UART_IIR_CTI) {
        uint8_t lsr = uart_read_reg(UART_LSR);
        while (lsr & UART_LSR_DR) {
            uint8_t b = uart_read_reg(UART_RBR);
            if (lsr & UART_LSR_ERR_MASK) {
                /* Bad byte: discard accumulated buffer, force resync */
                rxbuf_discard_all();
                rxbuf_overflowed = true;
            } else {
                rxbuf_push(b);
            }
            lsr = uart_read_reg(UART_LSR);
        }
        return;
    }
}

int uart_send_byte(uint8_t b) {
    int attempts = 10000;
    while (attempts--) {
        if (uart_read_reg(UART_LSR) & UART_LSR_THRE) {
            uart_write_reg(UART_THR, b);
            return 0;
        }
    }
    return 1;
}

int uart_send_buf(const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (uart_send_byte(buf[i]) != 0) return 1;
    }
    return 0;
}

bool uart_recv_byte(uint8_t *b) {
    return rxbuf_pop(b);
}

bool uart_rx_overflow(void) {
    bool v = rxbuf_overflowed;
    rxbuf_overflowed = false;
    return v;
}
