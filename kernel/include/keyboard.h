#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include "input.h"

void keyboard_init(void);
void keyboard_poll(void);
int  keyboard_read_key(void);
int  keyboard_check_key(void);
int  keyboard_get_shift_state(void);
int  keyboard_check_arrow(void);

#endif /* KEYBOARD_H */
