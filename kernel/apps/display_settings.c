#include "../include/display_settings.h"
#include "../include/tui.h"
#include "../include/input.h"
#include "../include/vga.h"
#include "../include/theme.h"

static int selected_theme_idx = 0;
static int night_light_enabled = 0;

static void display_settings_render(void) {
    aegis_theme_t* t = theme_get_current();
    
    tui_draw_frame("EKRAN VE GORUNUM AYARLARI");
    
    vga_set_cursor(5, 5);
    vga_puts("Mevcut Çozunurluk  : 1360 x 768 (Varsayilan)", t->fg_color);
    
    vga_set_cursor(5, 7);
    vga_puts("[ Temalar ]", t->accent_color);
    
    // Draw theme list
    for (int i = 0; i < 3; i++) {
        vga_set_cursor(7, 9 + i);
        if (i == selected_theme_idx) {
            vga_putc('>', t->accent_color);
            vga_putc(' ', t->bg_color);
        } else {
            vga_puts("  ", t->bg_color);
        }
        
        switch(i) {
            case 0: vga_puts("Neon Dark (Varsayilan)", t->fg_color); break;
            case 1: vga_puts("Aegis Classic", t->fg_color); break;
            case 2: vga_puts("High Contrast", t->fg_color); break;
        }
    }
    
    vga_set_cursor(5, 14);
    vga_puts("[ Gece Isigi (Mavi Isik Filtresi) ]", t->accent_color);
    vga_set_cursor(7, 16);
    if (night_light_enabled)
        vga_puts("Durum: ACIK (2700K)", t->fg_color);
    else
        vga_puts("Durum: KAPALI (6500K)", 0x08); // Dark grey
        
    vga_set_cursor(5, 19);
    vga_puts("GPU: Aegis Graphics X1", t->fg_color);
    
    tui_draw_bottom_bar("YON TUSLARI: Sec  ENTER: Uygula  N: Gece Isigi Ac/Kapat  ESC: Cikis");
    
    vga_swap_buffers();
}

void display_settings_open(void) {
    while(input_available()) input_get();
    
    display_settings_render();
    
    while(1) {
        if(input_available()) {
            key_event_t ev = input_get();
            if(!ev.pressed) continue;
            
            if (ev.key == KEY_ESC) {
                tui_clear_screen();
                vga_swap_buffers();
                return;
            } else if (ev.key == KEY_UP) {
                if (selected_theme_idx > 0) selected_theme_idx--;
                display_settings_render();
            } else if (ev.key == KEY_DOWN) {
                if (selected_theme_idx < 2) selected_theme_idx++;
                display_settings_render();
            } else if (ev.key == '\n' || ev.key == '\r') {
                theme_set(selected_theme_idx);
                // Also trigger an immediate re-render with new colors
                display_settings_render();
            } else if (ev.key == 'n' || ev.key == 'N') {
                night_light_enabled = !night_light_enabled;
                display_settings_render();
            }
        } else {
            for(volatile int i = 0; i < 5000; i++) __asm__ volatile("pause");
        }
    }
}
