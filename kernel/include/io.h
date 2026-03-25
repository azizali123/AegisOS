#ifndef IO_H
#define IO_H

#include <stdint.h>

/* ============================================================
   I/O Port Yardımcıları
   ============================================================ */

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

/* Donanım I/O gecikmesi (0x80 boş port) */
static inline void io_wait(void) {
    __asm__ volatile("outb %%al, $0x80" : : "a"(0));
}

/* Kelime dizisi oku/yaz */
static inline void insw(uint16_t port, void *addr, uint32_t cnt) {
    uint16_t *buf = (uint16_t *)addr;
    for (uint32_t i = 0; i < cnt; i++) buf[i] = inw(port);
}

static inline void outsw(uint16_t port, const void *addr, uint32_t cnt) {
    const uint16_t *buf = (const uint16_t *)addr;
    for (uint32_t i = 0; i < cnt; i++) outw(port, buf[i]);
}

#endif /* IO_H */
