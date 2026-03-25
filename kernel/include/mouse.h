#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

/* Fare butonu bitleri */
#define MOUSE_BTN_LEFT   0x01
#define MOUSE_BTN_RIGHT  0x02
#define MOUSE_BTN_MIDDLE 0x04

typedef struct {
    int x;           /* Mutlak X konumu */
    int y;           /* Mutlak Y konumu */
    int dx;          /* Son hareketteki X değişimi */
    int dy;          /* Son hareketteki Y değişimi */
    uint8_t buttons; /* Buton durumu bitmask */
    int scroll;      /* Scroll tekerleği (-1/0/+1) */
} MouseState;

void mouse_init(void);
void mouse_poll(void);
const MouseState* mouse_get_state(void);
int  mouse_clicked(uint8_t button);

#endif /* MOUSE_H */
