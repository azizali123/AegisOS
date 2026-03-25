#include "../include/theme.h"

aegis_theme_t themes[3] = {
    { "Neon Dark",      0x00, 0x0F, 0x0D, 0x05 }, // Black bg, White fg, Light Magenta accent, Magenta border
    { "Aegis Classic",  0x01, 0x0F, 0x0E, 0x09 }, // Blue bg, White fg, Yellow accent, Light Blue border
    { "High Contrast",  0x00, 0x0F, 0x0A, 0x0C }  // Black bg, White fg, Light Green accent, Light Red border
};

aegis_theme_t current_theme;

void theme_init(void) {
    // Default to Neon Dark
    current_theme = themes[0];
}

void theme_set(int theme_id) {
    if (theme_id >= 0 && theme_id < 3) {
        current_theme = themes[theme_id];
    }
}

aegis_theme_t* theme_get_current(void) {
    return &current_theme;
}
