#ifndef TUI_H
#define TUI_H

#include "theme.h"

void tui_init(void);

// TUI frame commands
void tui_clear_screen(void);
void tui_draw_border(void);
void tui_draw_frame(const char* title);
void tui_draw_bottom_bar(const char* info);

// Text styling
void tui_set_fg(uint8_t color);
void tui_set_bg(uint8_t color);

// Primitive draws
void tui_put_char(int x, int y, char c);
void tui_put_str(int x, int y, const char* str);
void tui_put_dec(int x, int y, uint32_t val);
void tui_put_hex(int x, int y, uint32_t val);

#endif
