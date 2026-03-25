#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

void  memory_init(void);
void *kmalloc(size_t size);
void  kfree(void *ptr);
void *kmalloc_aligned(size_t size, size_t align);
void  memory_print_stats(void);

void *kmemset(void *s, int c, size_t n);
void *kmemcpy(void *dst, const void *src, size_t n);
int   kmemcmp(const void *a, const void *b, size_t n);

#endif /* MEMORY_H */
