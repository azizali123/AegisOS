/* ============================================================
   session.c - Oturum Yonetimi
   Login ekrani, logout, oturum durumu
   ============================================================ */
#include "../include/session.h"
#include "../include/vga.h"
#include "../include/keyboard.h"
#include "../include/pit.h"
#include "../include/rtc.h"
#include <stdint.h>

static int logout_requested = 0;
static int session_active = 0;

/* Oturum sistemini baslat */
void session_init(void) {
    logout_requested = 0;
    session_active = 0;
}

/* Login ekranini ciz */
static void draw_login_screen(void) {
    vga_clear(0x00);
    int cols = (int)(vga_get_width() / 8);

    /* Logo */
    vga_set_cursor(cols/2 - 18, 5);
    vga_puts("====================================", 0x09);
    vga_set_cursor(cols/2 - 18, 6);
    vga_puts("||                                ||", 0x09);
    vga_set_cursor(cols/2 - 18, 7);
    vga_puts("||      A E G I S   O S  v4.0     ||", 0x0B);
    vga_set_cursor(cols/2 - 18, 8);
    vga_puts("||      UEFI / x86 Platform       ||", 0x0B);
    vga_set_cursor(cols/2 - 18, 9);
    vga_puts("||                                ||", 0x09);
    vga_set_cursor(cols/2 - 18, 10);
    vga_puts("====================================", 0x09);

    /* Hosgeldin mesaji */
    vga_set_cursor(cols/2 - 12, 13);
    vga_puts("Hosgeldiniz!", 0x0E);

    /* Tarih ve saat */
    uint8_t d, m, y, h, min, s;
    rtc_get_time(&d, &m, &y, &h, &min, &s);
    vga_set_cursor(cols/2 - 6, 15);
    char buf[3]; buf[2] = 0;
    buf[0] = '0' + d/10; buf[1] = '0' + d%10; vga_puts(buf, 0x07);
    vga_putc('/', 0x07);
    buf[0] = '0' + m/10; buf[1] = '0' + m%10; vga_puts(buf, 0x07);
    vga_putc('/', 0x07);
    vga_puts("20", 0x07);
    buf[0] = '0' + y/10; buf[1] = '0' + y%10; vga_puts(buf, 0x07);
    vga_puts("  ", 0x07);
    buf[0] = '0' + h/10; buf[1] = '0' + h%10; vga_puts(buf, 0x07);
    vga_putc(':', 0x07);
    buf[0] = '0' + min/10; buf[1] = '0' + min%10; vga_puts(buf, 0x07);

    /* Giris buton */
    vga_set_cursor(cols/2 - 18, 18);
    vga_puts("+------------------------------------+", 0x0B);
    vga_set_cursor(cols/2 - 18, 19);
    vga_puts("|  ENTER tusuna basarak giris yapin  |", 0x0F);
    vga_set_cursor(cols/2 - 18, 20);
    vga_puts("+------------------------------------+", 0x0B);

    /* Alt bilgi */
    vga_set_cursor(cols/2 - 15, 23);
    vga_puts("F1:Kapat  F2:Yeniden Baslat", 0x08);

    vga_swap_buffers();
}

/* Login ekrani goster */
int session_login_screen(void) {
    logout_requested = 0;
    draw_login_screen();

    /* Input buffer temizle */
    while (input_available()) input_get();

    while (1) {
        if (input_available()) {
            key_event_t ev = input_get();
            if (!ev.pressed) continue;
            int k = ev.key;

            if (k == '\n' || k == ' ') {
                session_active = 1;
                return 1; /* Basarili giris */
            }
            if (k == KEY_F1) {
                extern void power_shutdown(void);
                power_shutdown();
                /* Kapatma basarisiz olduysa tekrar login ekrani */
                draw_login_screen();
            }
            if (k == KEY_F2) {
                extern void power_reboot(void);
                power_reboot();
            }
        }

        /* Saat guncellemesi icin periyodik yeniden cizim */
        static uint64_t last_refresh = 0;
        uint64_t now = pit_get_millis();
        if (now - last_refresh > 60000) { /* Her 60 saniyede */
            last_refresh = now;
            draw_login_screen();
        }

        for (volatile int i = 0; i < 5000; i++)
            __asm__ volatile("pause");
        vga_swap_buffers();
    }
}

/* Oturum kapat */
void session_logout(void) {
    session_active = 0;
    logout_requested = 0;

    /* Cikis ekrani */
    vga_clear(0x00);
    int cols = (int)(vga_get_width() / 8);
    vga_set_cursor(cols/2 - 10, 12);
    vga_puts("Oturum kapatiliyor...", 0x0E);
    vga_swap_buffers();
    pit_sleep_ms(1000);
}

/* Logout istegini isaretle (shell icinden cagrilir) */
void session_request_logout(void) {
    logout_requested = 1;
}

/* Logout istegi var mi? */
int session_is_logout_requested(void) {
    return logout_requested;
}
