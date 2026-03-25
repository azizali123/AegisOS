#ifndef THEME_H
#define THEME_H

#include <stdint.h>

typedef struct {
    char     name[16];
    uint8_t  bg_color;
    uint8_t  fg_color;
    uint8_t  accent_color;
    uint8_t  border_color;
} aegis_theme_t;

extern aegis_theme_t current_theme;

void theme_init(void);
void theme_set(int theme_id);
aegis_theme_t* theme_get_current(void);

#endif
