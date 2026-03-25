#include "../include/network.h"
#include "../include/input.h"
#include "../include/vga.h"
#include "../include/tui.h"
#include <stdint.h>

/* ============================================================
   Firewall / Network UI (AegisOS v4.0)
   ============================================================ */

static void draw_network_stats(void) {
    aegis_theme_t* t = theme_get_current();
    net_status_t* status = network_get_status();
    net_stats_t* stats = network_get_stats();

    /* Ekranı temizle */
    for (int r = 2; r < 23; r++) {
        for (int c = 2; c < 78; c++) {
            vga_set_cursor(c, r);
            vga_putc(' ', t->bg_color);
        }
    }

    vga_set_cursor(4, 3);
    vga_puts("[AĞ BAĞLANTISI]", t->accent_color);

    vga_set_cursor(6, 5);
    vga_puts("Durum     : ", t->fg_color);
    if (status->connected) {
        vga_puts("Bağlı (", 0x0A);
        vga_puts(status->ssid, 0x0A);
        vga_puts(")", 0x0A);
    } else {
        vga_puts("Bağlantı Yok", 0x0C);
    }

    vga_set_cursor(6, 6);
    vga_puts("IP Adresi : ", t->fg_color);
    if (status->connected) {
        vga_put_dec(status->ip[0], t->fg_color); vga_putc('.', t->fg_color);
        vga_put_dec(status->ip[1], t->fg_color); vga_putc('.', t->fg_color);
        vga_put_dec(status->ip[2], t->fg_color); vga_putc('.', t->fg_color);
        vga_put_dec(status->ip[3], t->fg_color);
    } else {
        vga_puts("0.0.0.0", t->border_color);
    }

    vga_set_cursor(6, 7);
    vga_puts("MAC Adres : ", t->fg_color);
    for (int i = 0; i < 6; i++) {
        vga_put_hex(status->mac[i], t->fg_color);
        if (i < 5) vga_putc(':', t->fg_color);
    }
    if (status->mac_spoofed) {
        vga_puts(" (Spoofed/Anonim)", t->accent_color);
    }

    /* GÜVENLİK */
    vga_set_cursor(4, 10);
    vga_puts("[GÜVENLİK PROFİLİ]", t->accent_color);

    vga_set_cursor(6, 12);
    vga_puts("Firewall  : ", t->fg_color);
    if (status->firewall_active) {
        vga_puts("AKTİF [AÇIK] - Paket Filtreleme Devrede", 0x0A);
    } else {
        vga_puts("PASİF [KAPALI] - Gelen İstekler Kabul Ediliyor", 0x0C);
    }

    vga_set_cursor(6, 13);
    vga_puts("VPN (IPsec): ", t->fg_color);
    if (status->vpn_active) {
        vga_puts("AKTİF [AÇIK] - Tünellenmiş Bağlantı", 0x0A);
    } else {
        vga_puts("PASİF [KAPALI] - Standart ISP Rotası", 0x0C);
    }

    /* İSTATİSTİKLER */
    vga_set_cursor(4, 16);
    vga_puts("[TRAFİK İSTATİSTİKLERİ]", t->accent_color);

    vga_set_cursor(6, 18);
    vga_puts("Gönderilen Paket : ", t->fg_color);
    vga_put_dec((uint32_t)stats->tx_packets, t->accent_color);

    vga_set_cursor(6, 19);
    vga_puts("Alınan Paket     : ", t->fg_color);
    vga_put_dec((uint32_t)stats->rx_packets, t->accent_color);

    vga_set_cursor(40, 18);
    vga_puts("Gönderilen Veri  : ", t->fg_color);
    vga_put_dec((uint32_t)(stats->tx_bytes / 1024), t->accent_color);
    vga_puts(" KB", t->fg_color);

    vga_set_cursor(40, 19);
    vga_puts("Alınan Veri      : ", t->fg_color);
    vga_put_dec((uint32_t)(stats->rx_bytes / 1024), t->accent_color);
    vga_puts(" KB", t->fg_color);

    /* Güncelle */
    vga_swap_buffers();
}

void app_firewall(void) {
    tui_draw_frame("AĞ & GÜVENLİK MERKEZİ");
    tui_draw_bottom_bar("F1: FIREWALL AÇ/KAPAT    F2: VPN AÇ/KAPAT    F3: MAC DEĞİŞTİR    ESC: ÇIKIŞ");

    draw_network_stats();

    while (1) {
        key_event_t ev = input_get();
        if (!ev.pressed) continue;
        int k = ev.key;

        if (k == 27) { /* ESC */
            vga_clear(0x00);
            vga_swap_buffers();
            return;
        }

        if (k == KEY_F1) {
            network_firewall_toggle();
            draw_network_stats();
        }

        if (k == KEY_F2) {
            network_vpn_toggle();
            draw_network_stats();
        }

        if (k == KEY_F3) {
            network_mac_spoof();
            draw_network_stats();
        }
    }
}
