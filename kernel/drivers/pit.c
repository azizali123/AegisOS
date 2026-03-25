/* ============================================================
   pit.c - 8253/8254 PIT Timer Sürücüsü
   Kanal 0 → IRQ0 (sistem saati, 1000 Hz = 1ms)
   Kanal 2 → PC Speaker (audio.c tarafından kullanılır)
   ============================================================ */
#include "../include/pit.h"
#include "../include/vga.h"
#include <stdint.h>

#define PIT_CH0   0x40
#define PIT_CH2   0x42
#define PIT_MODE  0x43
#define PIT_FREQ  1193182UL

static volatile uint32_t pit_ticks   = 0;
static volatile uint64_t pit_millis_ = 0;
static uint32_t ticks_per_ms = 1;

static inline void pit_outb(uint16_t p, uint8_t v) {
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}

void pit_init(uint32_t freq_hz) {
    if (freq_hz == 0) freq_hz = 1000;
    ticks_per_ms = freq_hz / 1000;
    if (ticks_per_ms == 0) ticks_per_ms = 1;

    uint32_t divisor = (uint32_t)(PIT_FREQ / freq_hz);
    /* Kanal 0, mod 3 (kare dalga), ikili sayma */
    pit_outb(PIT_MODE, 0x36);
    pit_outb(PIT_CH0,  (uint8_t)(divisor & 0xFF));
    pit_outb(PIT_CH0,  (uint8_t)(divisor >> 8));
}

/* IRQ0 işleyicisinden çağrılır */
void pit_irq_handler(void) {
    pit_ticks++;
    if (pit_ticks % ticks_per_ms == 0) pit_millis_++;
    /* PIC EOI — Master PIC'e gönder */
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
}

uint32_t pit_get_ticks(void)  { return pit_ticks;   }
uint64_t pit_get_millis(void) { return pit_millis_;  }

void pit_sleep_ms(uint32_t ms) {
    uint64_t start = pit_millis_;
    while (pit_millis_ - start < ms) {
        vga_swap_buffers();
        __asm__ volatile("hlt"); /* CPU'yu IRQ'ya kadar durdur */
    }
    vga_swap_buffers(); /* Çıkmadan önce son kez güncelle */
}

void pit_sleep_ticks(uint32_t ticks) {
    uint32_t start = pit_ticks;
    while (pit_ticks - start < ticks) {
        vga_swap_buffers();
        __asm__ volatile("hlt"); /* CPU'yu IRQ'ya kadar durdur */
    }
    vga_swap_buffers();
}
