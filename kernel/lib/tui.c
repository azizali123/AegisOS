#include "../include/tui.h"
#include "../include/vga.h"
#include "../include/theme.h"

void tui_init(void) {
    theme_init();
}

void tui_clear_screen(void) {
    aegis_theme_t* t = theme_get_current();
    vga_clear(t->bg_color);
}

void tui_draw_border(void) {
    aegis_theme_t* t = theme_get_current();
    int w = 80; // Standard text width or vga_get_width()/8
    int h = 25; // Standard text height or vga_get_height()/16
    
    // Top & Bottom
    for (int x = 0; x < w; x++) {
        vga_set_cursor(x, 0);
        vga_putc('#', t->border_color);
        vga_set_cursor(x, h - 1);
        vga_putc('#', t->border_color);
    }
    // Left & Right
    for (int y = 0; y < h; y++) {
        vga_set_cursor(0, y);
        vga_putc('#', t->border_color);
        vga_set_cursor(w - 1, y);
        vga_putc('#', t->border_color);
    }
}

void tui_draw_frame(const char* title) {
    tui_clear_screen();
    tui_draw_border();
    
    aegis_theme_t* t = theme_get_current();
    
    // Top bar: ### [ TITLE ] [ _ ] [ # ] [ X ] ###
    int w = 80;
    
    // " ### [ TITLE ] " and control buttons
    char buf[64];
    int title_len = 0;
    while(title[title_len] && title_len < 32) title_len++;
    
    vga_set_cursor(2, 0);
    vga_putc('[', t->border_color);
    vga_putc(' ', t->bg_color);
    for(int i=0; i<title_len; i++) vga_putc(title[i], t->accent_color);
    vga_putc(' ', t->bg_color);
    vga_putc(']', t->border_color);
    
    int btn_x = w - 18;
    vga_set_cursor(btn_x, 0);
    vga_puts("[ _ ] [ # ] [ X ]", t->border_color);
}

void tui_draw_bottom_bar(const char* info) {
    aegis_theme_t* t = theme_get_current();
    int h = 25;
    
    vga_set_cursor(2, h-1);
    vga_puts("### [", t->border_color);
    vga_puts(info, t->fg_color);
    vga_puts("] ###", t->border_color);
}

void tui_put_char(int x, int y, char c) {
    aegis_theme_t* t = theme_get_current();
    vga_set_cursor(x, y);
    vga_putc(c, t->fg_color);
}

void tui_put_str(int x, int y, const char* str) {
    aegis_theme_t* t = theme_get_current();
    vga_set_cursor(x, y);
    vga_puts(str, t->fg_color);
}

void tui_put_dec(int x, int y, uint32_t val) {
    aegis_theme_t* t = theme_get_current();
    vga_set_cursor(x, y);
    vga_put_dec(val, t->fg_color);
}

void tui_put_hex(int x, int y, uint32_t val) {
    aegis_theme_t* t = theme_get_current();
    vga_set_cursor(x, y);
    vga_put_hex(val, t->fg_color);
}
