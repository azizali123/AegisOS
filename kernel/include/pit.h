#ifndef PIT_H
#define PIT_H

#include <stdint.h>

/* PIT başlatma (1000 Hz = 1ms tick) */
void pit_init(uint32_t freq_hz);

/* Tick sayacı */
uint32_t pit_get_ticks(void);
uint64_t pit_get_millis(void);

/* Gecikme fonksiyonları */
void pit_sleep_ms(uint32_t ms);
void pit_sleep_ticks(uint32_t ticks);

/* IRQ0 kesme işleyicisi (idt.c'den çağrılır) */
void pit_irq_handler(void);

#endif /* PIT_H */
