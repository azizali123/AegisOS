/* ============================================================
   memory.c - Basit Bump Allocator
   Sabit 4MB heap, hizalamalı kmalloc desteği
   ============================================================ */
#include "../include/memory.h"
#include "../include/vga.h"
#include <stdint.h>
#include <stddef.h>

#define HEAP_SIZE (4 * 1024 * 1024)  /* 4 MB */

uint8_t heap[HEAP_SIZE] __attribute__((aligned(16)));
uint32_t heap_ptr = 0;

void memory_init(void) {
    heap_ptr = 0;
    for (uint32_t i = 0; i < 16; i++) heap[i] = 0; /* dokunma testi */
    vga_puts("  [MEM] Heap baslatildi (4MB)\n", 0x0A);
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    /* 8-byte hizala */
    uint32_t aligned = (heap_ptr + 7) & ~7u;
    if (aligned + size > HEAP_SIZE) return NULL;
    heap_ptr = aligned + (uint32_t)size;
    return (void *)(heap + aligned);
}

void *kmalloc_aligned(size_t size, size_t align) {
    if (size == 0 || align == 0) return NULL;
    uint32_t mask = (uint32_t)(align - 1);
    uint32_t aligned = (heap_ptr + mask) & ~mask;
    if (aligned + size > HEAP_SIZE) return NULL;
    heap_ptr = aligned + (uint32_t)size;
    return (void *)(heap + aligned);
}

void kfree(void *ptr) { (void)ptr; /* Bump allocator serbest bırakma yok */ }

void memory_print_stats(void) {
    vga_puts("  Heap: ", 0x07);
    vga_put_dec(heap_ptr, 0x0F);
    vga_puts(" / ", 0x07);
    vga_put_dec(HEAP_SIZE, 0x0F);
    vga_puts(" byte kullanildi\n", 0x07);
}

/* ---- Bellek yardımcıları ---- */
void *kmemset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}
void *kmemcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dst;
}
int kmemcmp(const void *a, const void *b, size_t n) {
    const uint8_t *p = (const uint8_t *)a, *q = (const uint8_t *)b;
    while (n--) { if (*p != *q) return *p - *q; p++; q++; }
    return 0;
}
