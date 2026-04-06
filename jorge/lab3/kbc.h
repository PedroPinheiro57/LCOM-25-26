#ifndef _LCOM_KBC_H_
#define _LCOM_KBC_H_

#include <stdbool.h>
#include <stdint.h>


int kbd_subscribe_int(uint8_t *bit_no);

int kbd_unsubscribe_int();

void (kbc_ih)();

uint8_t kbd_get_scancode();

bool kbd_has_error();

#endif /* _LCOM_KBC_H_ */
