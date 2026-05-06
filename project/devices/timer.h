#pragma once
#include <lcom/timer.h>

/* timer_get_counter and timer_reset_counter are not in lcom/timer.h */
uint32_t timer_get_counter(void);
void     timer_reset_counter(void);
