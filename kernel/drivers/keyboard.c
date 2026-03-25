/* ============================================================
   keyboard.c - Evrensel PS/2 Klavye Sürücüsü
   TR-Q (F klavye de eklenebilir), Shift, CapsLock, F1-F12,
   Yön tuşları, Home/End/PgUp/PgDn/Ins/Del
   ============================================================ */
#include "../include/keyboard.h"
#include "../include/input.h"
#include <stdint.h>

#define PS2_DATA   0x60
#define PS2_STATUS 0x64
#define PS2_OBF    0x01   /* Output Buffer Full */
#define PS2_IBF    0x02   /* Input Buffer Full  */
#define PS2_MOUSE  0x20   /* Fare verisi bayrağı */

/* ---- Tuş tamponu ---- */
#define KEY_BUF_SIZE 64
static key_event_t key_buffer[KEY_BUF_SIZE];
static volatile int buf_head = 0, buf_tail = 0;

static inline void buf_push(key_event_t ev) {
    int next = (buf_head + 1) % KEY_BUF_SIZE;
    if (next != buf_tail) { key_buffer[buf_head] = ev; buf_head = next; }
}
static inline int buf_pop(key_event_t *ev) {
    if (buf_head == buf_tail) return 0;
    *ev = key_buffer[buf_tail];
    buf_tail = (buf_tail + 1) % KEY_BUF_SIZE;
    return 1;
}

static inline uint8_t ps2_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1,%0":"=a"(ret):"Nd"(port));
    return ret;
}
static inline void ps2_outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0,%1"::"a"(val),"Nd"(port));
}
static inline void io_wait(void) {
    __asm__ volatile("outb %%al,$0x80"::"a"(0));
}
static void ps2_wait_write(void) {
    int t = 100000;
    while ((ps2_inb(PS2_STATUS) & PS2_IBF) && t--) io_wait();
}

/* ---- Durum ---- */
volatile int key_state[512] = {0};

static int shift_pressed = 0;
static int ctrl_pressed  = 0;
static int alt_pressed   = 0;
static int caps_lock     = 0;
static int num_lock      = 0;
static int e0_flag       = 0;

/* ============================================================
   Türkçe Q klavye tablosu (Scancode Set 1)
   ============================================================ */
static const unsigned char tr_lower[128] = {
    [0x01]=27,   [0x02]='1',  [0x03]='2',  [0x04]='3',
    [0x05]='4',  [0x06]='5',  [0x07]='6',  [0x08]='7',
    [0x09]='8',  [0x0A]='9',  [0x0B]='0',  [0x0C]='*',
    [0x0D]='-',  [0x0E]='\b', [0x0F]='\t',
    [0x10]='q',  [0x11]='w',  [0x12]='e',  [0x13]='r',
    [0x14]='t',  [0x15]='y',  [0x16]='u',  [0x17]=133, /* ı */
    [0x18]='o',  [0x19]='p',  [0x1A]=129, /* ğ */
    [0x1B]=132, /* ü */       [0x1C]='\n',
    [0x1E]='a',  [0x1F]='s',  [0x20]='d',  [0x21]='f',
    [0x22]='g',  [0x23]='h',  [0x24]='j',  [0x25]='k',
    [0x26]='l',  [0x27]=130, /* ş */      [0x28]='i',
    [0x29]='"',  [0x2B]=',',  [0x56]='<',
    [0x2C]='z',  [0x2D]='x',  [0x2E]='c',  [0x2F]='v',
    [0x30]='b',  [0x31]='n',  [0x32]='m',
    [0x33]=131, /* ö */       [0x34]=128, /* ç */
    [0x35]='.',  [0x39]=' ',
};
static const unsigned char tr_upper[128] = {
    [0x01]=27,   [0x02]='!',  [0x03]='\'', [0x04]='^',
    [0x05]='+',  [0x06]='%',  [0x07]='&',  [0x08]='/',
    [0x09]='(',  [0x0A]=')',  [0x0B]='=',  [0x0C]='?',
    [0x0D]='_',  [0x0E]='\b', [0x0F]='\t',
    [0x10]='Q',  [0x11]='W',  [0x12]='E',  [0x13]='R',
    [0x14]='T',  [0x15]='Y',  [0x16]='U',  [0x17]='I', /* I */
    [0x18]='O',  [0x19]='P',  [0x1A]=139, /* Ğ */
    [0x1B]=137, /* Ü */       [0x1C]='\n',
    [0x1E]='A',  [0x1F]='S',  [0x20]='D',  [0x21]='F',
    [0x22]='G',  [0x23]='H',  [0x24]='J',  [0x25]='K',
    [0x26]='L',  [0x27]=136, /* Ş */      [0x28]=134, /* İ */
    [0x29]='E',  [0x2B]=';',  [0x56]='>',
    [0x2C]='Z',  [0x2D]='X',  [0x2E]='C',  [0x2F]='V',
    [0x30]='B',  [0x31]='N',  [0x32]='M',
    [0x33]=138, /* Ö */       [0x34]=135, /* Ç */
    [0x35]=':',  [0x39]=' ',
};

static void process_scancode(uint8_t sc, key_event_t *ev_out) {
    int is_release = (sc & 0x80) ? 1 : 0;
    uint8_t code = sc & 0x7F;
    ev_out->pressed = !is_release;
    ev_out->key = 0;

    int mods = 0;
    if (shift_pressed) mods |= MOD_SHIFT;
    if (ctrl_pressed)  mods |= MOD_CTRL;
    if (alt_pressed)   mods |= MOD_ALT;
    ev_out->modifiers = mods;

    /* Genişletilmiş ön ek */
    if (sc == 0xE0) { e0_flag = 1; ev_out->key = 0; return; }

    /* Modifier tuşları */
    if (code == 0x2A || code == 0x36) { shift_pressed = !is_release; ev_out->key = 0; return; }
    if (code == 0x1D) { ctrl_pressed = !is_release; ev_out->key = 0; return; }
    if (code == 0x38) { alt_pressed  = !is_release; ev_out->key = 0; return; }
    if (!is_release && code == 0x3A) { caps_lock = !caps_lock; ev_out->key = 0; return; }
    if (!is_release && code == 0x45) { num_lock  = !num_lock;  ev_out->key = 0; return; }

    /* E0 genişletilmiş tuşlar (yön, Home, End…) */
    if (e0_flag) {
        e0_flag = 0;
        switch (code) {
            case 0x48: ev_out->key = KEY_UP; return;
            case 0x50: ev_out->key = KEY_DOWN; return;
            case 0x4B: ev_out->key = KEY_LEFT; return;
            case 0x4D: ev_out->key = KEY_RIGHT; return;
            case 0x47: ev_out->key = KEY_HOME; return;
            case 0x4F: ev_out->key = KEY_END; return;
            case 0x49: ev_out->key = KEY_PGUP; return;
            case 0x51: ev_out->key = KEY_PGDN; return;
            case 0x52: ev_out->key = KEY_INS; return;
            case 0x53: ev_out->key = KEY_DEL; return;
            case 0x1C: ev_out->key = '\n'; return;
            case 0x35: ev_out->key = '/'; return;
        }
    }

    if (code == 0x48) { ev_out->key = (!num_lock && !shift_pressed) ? KEY_UP : '8'; return; }
    if (code == 0x50) { ev_out->key = (!num_lock && !shift_pressed) ? KEY_DOWN : '2'; return; }
    if (code == 0x4B) { ev_out->key = (!num_lock && !shift_pressed) ? KEY_LEFT : '4'; return; }
    if (code == 0x4D) { ev_out->key = (!num_lock && !shift_pressed) ? KEY_RIGHT : '6'; return; }
    if (code == 0x47) { ev_out->key = (!num_lock && !shift_pressed) ? KEY_HOME : '7'; return; }
    if (code == 0x4F) { ev_out->key = (!num_lock && !shift_pressed) ? KEY_END : '1'; return; }
    if (code == 0x49) { ev_out->key = (!num_lock && !shift_pressed) ? KEY_PGUP : '9'; return; }
    if (code == 0x51) { ev_out->key = (!num_lock && !shift_pressed) ? KEY_PGDN : '3'; return; }
    if (code == 0x52) { ev_out->key = (!num_lock && !shift_pressed) ? KEY_INS : '0'; return; }
    if (code == 0x53) { ev_out->key = (!num_lock && !shift_pressed) ? KEY_DEL : '.'; return; }

    /* F1-F12 */
    if (code >= 0x3B && code <= 0x44) { ev_out->key = KEY_F1 + (code - 0x3B); return; }
    if (code == 0x57) { ev_out->key = KEY_F11; return; }
    if (code == 0x58) { ev_out->key = KEY_F12; return; }

    /* ASCII karakterler */
    if (code < 128) {
        unsigned char lo = tr_lower[code];
        int is_letter = (lo >= 'a' && lo <= 'z') || lo == 133 || lo == 129 || lo == 132 || lo == 130 || lo == 131 || lo == 128;
        int use_upper = shift_pressed;
        if (caps_lock && is_letter) use_upper = !use_upper;

        unsigned char ascii = use_upper ? tr_upper[code] : lo;

        /* Ctrl kombinasyonları */
        if (ctrl_pressed && ascii >= 'a' && ascii <= 'z') {
            ev_out->key = ascii - 'a' + 1;
        } else {
            ev_out->key = ascii ? (int)ascii : 0;
        }
    }
}

/* ============================================================
   Başlatma
   ============================================================ */
void keyboard_init(void) {
    /* Output buffer'ı temizle */
    int timeout = 100000;
    while ((ps2_inb(PS2_STATUS) & PS2_OBF) && timeout-- > 0)
        ps2_inb(PS2_DATA);

    /* Klavyeyi etkinleştir (0xAE komutu) */
    ps2_wait_write();
    ps2_outb(PS2_STATUS, 0xAE);
    io_wait();
}

/* ============================================================
   Polling
   ============================================================ */
void keyboard_poll(void) {
    int max = 32;
    while (max-- > 0) {
        uint8_t status = ps2_inb(PS2_STATUS);
        if (!(status & PS2_OBF)) break;
        if (status & PS2_MOUSE) { ps2_inb(PS2_DATA); continue; } /* fare verisi → atla */
        uint8_t sc = ps2_inb(PS2_DATA);
        
        key_event_t ev;
        process_scancode(sc, &ev);
        if (ev.key != 0) {
            if (ev.key < 512) key_state[ev.key] = ev.pressed;
            if (ev.pressed) buf_push(ev);
        }
    }
}

/* Boşta döngü kancası — SMM / USB Legacy emülasyonunu boğmamak için bekleme */
void __attribute__((weak)) os_idle_hook(void) {
    for (volatile int i = 0; i < 100000; i++)
        __asm__ volatile("pause");
}

/* ============================================================
   Input Manager Backend API
   ============================================================ */
int input_check(key_event_t *event) {
    keyboard_poll();
    return buf_pop(event);
}

int input_available(void) {
    keyboard_poll();
    return (buf_head != buf_tail);
}

key_event_t input_get(void) {
    key_event_t ev;
    while (1) {
        os_idle_hook();
        if (input_check(&ev)) {
            return ev;
        }
    }
}

/* Geri uyumluluk için, eskisini kullanan kodlar build hatası vermemesi geçici olarak input_get sarmalanabilir, ancak eski yerleri input_get ile değiştireceğiz */
int keyboard_read_key(void) {
    key_event_t e = input_get();
    return e.key;
}
int keyboard_check_key(void) {
    key_event_t ev;
    if (input_check(&ev)) return ev.key;
    return 0;
}
int keyboard_get_shift_state(void) { return shift_pressed; }
int __attribute__((weak)) keyboard_check_arrow(void) { return keyboard_check_key(); }

