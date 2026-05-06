#pragma once
#include <stdint.h>

typedef struct {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
} rtc_time_t;

typedef struct {
  uint8_t day;
  uint8_t month;
  uint8_t year;
} rtc_date_t;

int rtc_read_time(rtc_time_t *t);
int rtc_read_date(rtc_date_t *d);
