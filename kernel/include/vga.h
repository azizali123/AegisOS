#ifndef VGA_H
#define VGA_H

#include <stdint.h>

/* Grafik modu başlatma (Multiboot2 framebuffer) */
void vga_init_graphics(void *mb2_info);

/* Temel çizim */
void vga_putc(char c, uint8_t color);
void vga_puts(const char *s, uint8_t color);
void vga_put_hex(uint32_t val, uint8_t color);
void vga_put_dec(uint32_t val, uint8_t color);
void vga_put_int(int val, uint8_t color);
void vga_newline(void);
void vga_clear(uint8_t color);
void vga_set_cursor(int x, int y);
void vga_get_cursor(int *x, int *y);
void vga_scroll(void);

/* Gelişmiş çizim */
void vga_draw_pixel(int x, int y, uint32_t color);
void vga_draw_rect(int x, int y, int w, int h, uint32_t color);
void vga_draw_rect_fill(int x, int y, int w, int h, uint32_t color);
void vga_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
uint32_t vga_get_width(void);
uint32_t vga_get_height(void);

/* Çift Tamponlama (Double Buffering) */
void vga_swap_buffers(void);

/* Renk sabitleri (ARGB) */
#define COLOR_BLACK   0x00
#define COLOR_BLUE    0x01
#define COLOR_GREEN   0x02
#define COLOR_CYAN    0x03
#define COLOR_RED     0x04
#define COLOR_MAGENTA 0x05
#define COLOR_BROWN   0x06
#define COLOR_LGRAY   0x07
#define COLOR_DGRAY   0x08
#define COLOR_LBLUE   0x09
#define COLOR_LGREEN  0x0A
#define COLOR_LCYAN   0x0B
#define COLOR_LRED    0x0C
#define COLOR_LMAG    0x0D
#define COLOR_YELLOW  0x0E
#define COLOR_WHITE   0x0F

#endif /* VGA_H */
