#ifndef _LCOM_KBC_H_
#define _LCOM_KBC_H_

#pragma once
#include <stdbool.h>
#include <stdint.h>


int kbd_subscribe_int(uint8_t *bit_no);

int kbd_unsubscribe_int();

void (kbc_ih)();

uint8_t kbd_get_scancode();

bool kbd_has_error();

int kbc_read_data_poll(uint8_t *data);

int kbc_write_command(uint8_t port, uint8_t cmd);

int kbc_restore_keyboard_interrupts();

void kbc_reset_sysinb_count(void);
uint32_t kbc_get_sysinb_count(void);

#endif /* _LCOM_KBC_H_ */
