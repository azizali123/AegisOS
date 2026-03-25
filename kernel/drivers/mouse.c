/* ============================================================
   mouse.c - PS/2 Fare Sürücüsü (Evrensel)
   Destekler: PS/2 fare, 3 buton, scroll tekerleği
   ============================================================ */
#include "../include/mouse.h"
#include "../include/vga.h"
#include <stdint.h>

#define PS2_DATA    0x60
#define PS2_STATUS  0x64
#define PS2_OBF     0x01
#define PS2_IBF     0x02
#define PS2_MOUSE   0x20

static MouseState mouse_state = {0, 0, 0, 0, 0, 0};
static int  mouse_cycle  = 0;
static uint8_t mouse_bytes[4];
static int  mouse_ready  = 0;
static int  mouse_scroll_wheel = 0;

static inline uint8_t mouse_inb(uint16_t port) {
    uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(port)); return r;
}
static inline void mouse_outb(uint16_t port, uint8_t v) {
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(port));
}
static inline void io_w(void) { __asm__ volatile("outb %%al,$0x80"::"a"(0)); }

static void mouse_wait_write(void) {
    int t = 100000;
    while ((mouse_inb(PS2_STATUS) & PS2_IBF) && t--) io_w();
}
static void mouse_wait_read(void) {
    int t = 100000;
    while (!(mouse_inb(PS2_STATUS) & PS2_OBF) && t--) io_w();
}

static void mouse_send_cmd(uint8_t cmd) {
    mouse_wait_write(); mouse_outb(PS2_STATUS, 0xD4);
    mouse_wait_write(); mouse_outb(PS2_DATA, cmd);
}

static uint8_t mouse_recv(void) {
    mouse_wait_read();
    return mouse_inb(PS2_DATA);
}

void mouse_init(void) {
    /* Fare arayüzünü etkinleştir */
    mouse_wait_write(); mouse_outb(PS2_STATUS, 0xA8);

    /* PS/2 konfigürasyon byte'ını oku */
    mouse_wait_write(); mouse_outb(PS2_STATUS, 0x20);
    mouse_wait_read();
    uint8_t cfg = mouse_inb(PS2_DATA);
    cfg |= 0x02;    /* Fare IRQ12 etkinleştir */
    cfg &= ~0x20;   /* Fare saatini aç */

    /* Konfigürasyonu yaz */
    mouse_wait_write(); mouse_outb(PS2_STATUS, 0x60);
    mouse_wait_write(); mouse_outb(PS2_DATA, cfg);

    /* Fareyi varsayılana sıfırla */
    mouse_send_cmd(0xFF);
    mouse_recv(); /* ACK veya 0xAA */
    mouse_recv(); /* 0x00 */

    /* Veri paketini etkinleştir */
    mouse_send_cmd(0xF4);
    mouse_recv(); /* ACK */

    /* Scroll tekerleği denetle (IntelliMouse) */
    mouse_send_cmd(0xF3); mouse_recv(); mouse_send_cmd(200); mouse_recv();
    mouse_send_cmd(0xF3); mouse_recv(); mouse_send_cmd(100); mouse_recv();
    mouse_send_cmd(0xF3); mouse_recv(); mouse_send_cmd(80);  mouse_recv();
    mouse_send_cmd(0xF2); mouse_recv();
    uint8_t dev_id = mouse_recv();
    if (dev_id == 3) { mouse_scroll_wheel = 1; }

    mouse_ready = 1;
    mouse_state.x = (int)(vga_get_width()  / 2);
    mouse_state.y = (int)(vga_get_height() / 2);
}

static void mouse_process_packet(void) {
    uint8_t b0 = mouse_bytes[0];
    uint8_t b1 = mouse_bytes[1];
    uint8_t b2 = mouse_bytes[2];

    /* Butonlar */
    mouse_state.buttons = b0 & 0x07;

    /* Hareket (işaretli 9-bit) */
    int dx = (int)(int8_t)b1;
    int dy = -(int)(int8_t)b2;  /* Y ekseni ters */

    /* Overflow kontrol */
    if (b0 & 0x40) dx = 0;
    if (b0 & 0x80) dy = 0;

    mouse_state.dx = dx;
    mouse_state.dy = dy;
    mouse_state.x += dx;
    mouse_state.y += dy;

    /* Ekran sınırları */
    int W = (int)vga_get_width();
    int H = (int)vga_get_height();
    if (mouse_state.x < 0)   mouse_state.x = 0;
    if (mouse_state.y < 0)   mouse_state.y = 0;
    if (mouse_state.x >= W)  mouse_state.x = W - 1;
    if (mouse_state.y >= H)  mouse_state.y = H - 1;

    /* Scroll tekerleği */
    if (mouse_scroll_wheel && mouse_bytes[3]) {
        mouse_state.scroll = (mouse_bytes[3] & 0x80) ? -1 : 1;
    } else {
        mouse_state.scroll = 0;
    }
}

void mouse_poll(void) {
    if (!mouse_ready) return;
    int max = 32;
    while (max-- > 0) {
        uint8_t status = mouse_inb(PS2_STATUS);
        if (!(status & PS2_OBF)) break;
        if (!(status & PS2_MOUSE)) { mouse_inb(PS2_DATA); continue; } /* klavye verisi → atla */

        uint8_t data = mouse_inb(PS2_DATA);
        int packet_size = mouse_scroll_wheel ? 4 : 3;

        /* İlk byte: bit 3 her zaman 1 olmalı (senkronizasyon) */
        if (mouse_cycle == 0 && !(data & 0x08)) continue;

        mouse_bytes[mouse_cycle++] = data;
        if (mouse_cycle >= packet_size) {
            mouse_process_packet();
            mouse_cycle = 0;
        }
    }
}

const MouseState* mouse_get_state(void) {
    mouse_poll();
    return &mouse_state;
}

int mouse_clicked(uint8_t button) {
    return (mouse_state.buttons & button) ? 1 : 0;
}
