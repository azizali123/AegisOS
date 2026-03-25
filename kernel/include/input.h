#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

/* Özel tuş kodları */
#define KEY_UP    0x100
#define KEY_DOWN  0x101
#define KEY_LEFT  0x102
#define KEY_RIGHT 0x103
#define KEY_HOME  0x104
#define KEY_END   0x105
#define KEY_PGUP  0x106
#define KEY_PGDN  0x107
#define KEY_INS   0x108
#define KEY_DEL   0x109
#define KEY_F1    0x110
#define KEY_F2    0x111
#define KEY_F3    0x112
#define KEY_F4    0x113
#define KEY_F5    0x114
#define KEY_F6    0x115
#define KEY_F7    0x116
#define KEY_F8    0x117
#define KEY_F9    0x118
#define KEY_F10   0x119
#define KEY_F11   0x11A
#define KEY_F12   0x11B
#define KEY_ESC   0x1B

/* Modifier Bayrakları */
#define MOD_SHIFT 0x01
#define MOD_CTRL  0x02
#define MOD_ALT   0x04

typedef struct {
    int key;         /* Basılan veya bırakılan tuş (ASCII veya KEY_XXX) */
    int pressed;     /* 1: Basıldı, 0: Bırakıldı */
    int modifiers;   /* MOD_SHIFT, MOD_CTRL vb. durumları */
} key_event_t;

/* Input API */
extern volatile int key_state[512];
key_event_t input_get(void);
int input_check(key_event_t *event);
int input_available(void);

#endif /* INPUT_H */
