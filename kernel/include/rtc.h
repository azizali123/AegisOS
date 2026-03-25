#ifndef RTC_H
#define RTC_H
#include <stdint.h>
void rtc_print_time(void);
void rtc_set_time(uint8_t day, uint8_t month, uint8_t year,
                  uint8_t hour, uint8_t min, uint8_t sec);
void rtc_get_time(uint8_t *day, uint8_t *month, uint8_t *year,
                  uint8_t *hour, uint8_t *min, uint8_t *sec);
#endif
