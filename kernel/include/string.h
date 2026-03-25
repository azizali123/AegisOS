#ifndef STRING_H
#define STRING_H
#include <stdint.h>
#include <stddef.h>
size_t strlen(const char *s);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);
char  *strcat(char *dst, const char *src);
char  *strchr(const char *s, int c);
int    katoi(const char *s);
void   kitoa(int val, char *buf, int base);
void   kuitoa(uint32_t val, char *buf, int base);
#endif
