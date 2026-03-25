/* ============================================================
   power.c - Guc Yonetimi
   Kapatma ve Yeniden Baslatma
   ============================================================ */
#include "../include/power.h"
#include "../include/vga.h"
#include "../include/keyboard.h"
#include "../include/pit.h"
#include <stdint.h>

/* I/O port erisimi */
static inline void pw_outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t pw_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Kapatma ekrani ciz */
static void draw_shutdown_screen(const char *msg) {
    vga_clear(0x00);
    int cols = (int)(vga_get_width() / 8);

    vga_set_cursor(cols/2 - 15, 10);
    vga_puts("==============================", 0x04);
    vga_set_cursor(cols/2 - 15, 11);
    vga_puts("||                          ||", 0x04);
    vga_set_cursor(cols/2 - 15, 12);
    vga_puts("||     AegisOS  v4.0        ||", 0x0C);
    vga_set_cursor(cols/2 - 15, 13);
    vga_puts("||                          ||", 0x04);
    vga_set_cursor(cols/2 - 15, 14);
    vga_puts("==============================", 0x04);

    vga_set_cursor(cols/2 - 20, 16);
    vga_puts(msg, 0x07);

    vga_swap_buffers();
}

/* Sistemi kapat */
void power_shutdown(void) {
    draw_shutdown_screen("Sistem kapatiliyor...");
    pit_sleep_ms(1000);

    /* ACPI kapatma denemesi - QEMU / Bochs uyumlu */
    /* QEMU: 0x604 portuna 0x2000 yazarak kapatma */
    pw_outb(0x604, 0x00);  /* QEMU i440fx */
    __asm__ volatile("outw %0, %1" : : "a"((uint16_t)0x2000), "Nd"((uint16_t)0x604));

    /* Bochs: 0xB004 portuna 0x2000 */
    __asm__ volatile("outw %0, %1" : : "a"((uint16_t)0x2000), "Nd"((uint16_t)0xB004));

    /* VirtualBox: 0x4004 portuna 0x3400 */
    __asm__ volatile("outw %0, %1" : : "a"((uint16_t)0x3400), "Nd"((uint16_t)0x4004));

    /* Eger hala calisiyorsa - ACPI desteklenmiyor */
    pit_sleep_ms(500);

    draw_shutdown_screen("ACPI kapatma desteklenmiyor.");
    vga_set_cursor(20, 18);
    vga_puts("Bilgisayarinizi guvenle kapatabilirsiniz.", 0x08);
    vga_set_cursor(20, 19);
    vga_puts("Herhangi bir tusa basin...", 0x08);
    vga_swap_buffers();

    /* Kesmeleri kapat ve bekle */
    while (1) {
        if (input_available()) {
            input_get();
            return; /* Shell'e geri don */
        }
        __asm__ volatile("hlt");
    }
}

/* Sistemi yeniden baslat */
void power_reboot(void) {
    draw_shutdown_screen("Sistem yeniden baslatiliyor...");
    pit_sleep_ms(1000);

    /* Yontem 1: Klavye denetleyicisi sifirlama (8042) */
    /* 0x64 portundaki durum kaydini kontrol et, CPU reset pulsunu gonder */
    uint8_t good = 0x02;
    while (good & 0x02)
        good = pw_inb(0x64);
    pw_outb(0x64, 0xFE); /* Reset komutu */

    /* Yontem 2: Triple fault (son care) */
    pit_sleep_ms(500);

    /* Triple fault ile zorla yeniden baslat */
    /* IDT'yi gecersiz yap */
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) null_idt = { 0, 0 };
    __asm__ volatile("lidt %0" : : "m"(null_idt));
    __asm__ volatile("int $3"); /* Triple fault tetikle */

    /* Buraya ulasilmamali */
    while (1) __asm__ volatile("hlt");
}
