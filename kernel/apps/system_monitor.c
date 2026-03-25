#include "../include/system_monitor.h"
#include "../include/input.h"
#include "../include/pci.h"
#include "../include/vga.h"
#include "../include/pit.h"
#include "../include/tui.h"
#include <stdint.h>

/* ============================================================
   Sistem Izlencesi (Task Manager)
   CPU, RAM, GPU panelleri - canli tarihce grafigi
   ============================================================ */

/* Dis referanslar */
extern volatile uint64_t idle_ticks_total;
extern volatile uint64_t busy_ticks_total;
extern uint32_t heap_ptr;
#define HEAP_SIZE (4 * 1024 * 1024)

/* Tarihce buffer */
#define HISTORY_SIZE 60
static uint32_t cpu_history[HISTORY_SIZE];
static uint32_t ram_history[HISTORY_SIZE];
static int history_idx = 0;

/* CPU olcumu */
static uint64_t prev_idle = 0;
static uint64_t prev_total = 0;
static uint32_t cpu_usage_pct = 0;

/* PCI GPU bilgisi */
static uint16_t gpu_vendor = 0;
static uint16_t gpu_device = 0;
static uint8_t  gpu_found = 0;

/* PCI taramasiyla GPU bul */
static void find_gpu_info(void) {
    gpu_found = 0;
    for (int bus = 0; bus < 256 && !gpu_found; bus++) {
        for (int slot = 0; slot < 32 && !gpu_found; slot++) {
            uint16_t vid = pci_config_read_word((uint8_t)bus, (uint8_t)slot, 0, 0);
            if (vid == 0xFFFF) continue;
            uint32_t cr = pci_config_read_dword((uint8_t)bus, (uint8_t)slot, 0, 0x08);
            uint8_t cls = (uint8_t)((cr >> 24) & 0xFF);
            /* Class 0x03 = Display Controller */
            if (cls == 0x03) {
                gpu_vendor = vid;
                gpu_device = pci_config_read_word((uint8_t)bus, (uint8_t)slot, 0, 2);
                gpu_found = 1;
            }
        }
    }
}

/* CPU kullanim oranini hesapla */
static void update_cpu_usage(void) {
    uint64_t current_idle = idle_ticks_total;
    uint64_t current_total = pit_get_ticks();

    uint64_t idle_diff = current_idle - prev_idle;
    uint64_t total_diff = current_total - prev_total;

    if (total_diff > 0) {
        /* Idle tick oranini hesapla */
        uint64_t busy = total_diff > idle_diff ? total_diff - idle_diff : 0;
        cpu_usage_pct = (uint32_t)((busy * 100) / total_diff);
        if (cpu_usage_pct > 100) cpu_usage_pct = 100;
    }

    prev_idle = current_idle;
    prev_total = current_total;
}

/* Heap kullanimini oku */
static uint32_t get_heap_used(void) {
    return heap_ptr;
}

static uint32_t get_heap_pct(void) {
    return (heap_ptr * 100) / HEAP_SIZE;
}

/* Tarihce guncelle */
static void update_history(void) {
    cpu_history[history_idx] = cpu_usage_pct;
    ram_history[history_idx] = get_heap_pct();
    history_idx = (history_idx + 1) % HISTORY_SIZE;
}

/* ---- CIZIM FONKSIYONLARI ---- */

/* Yatay bar grafigi ciz */
static void draw_bar(int x, int y, int width, uint32_t pct, uint8_t fill_color) {
    int filled = (int)((pct * (uint32_t)width) / 100);
    if (filled > width) filled = width;

    vga_set_cursor(x, y);
    vga_putc('[', 0x0F);
    for (int i = 0; i < width; i++) {
        if (i < filled)
            vga_putc(0xDB, fill_color);
        else
            vga_putc(0xB0, 0x08);
    }
    vga_putc(']', 0x0F);
}

/* Kucuk tarihce grafigi ciz (son 30 deger, blok stili) */
static void draw_history_graph(int x, int y, int w, int h, uint32_t *hist, uint8_t color) {
    /* Arka plan temizle */
    for (int r = 0; r < h; r++) {
        vga_set_cursor(x, y + r);
        for (int c = 0; c < w; c++)
            vga_putc(' ', 0x00);
    }

    /* Cerceve */
    for (int c = 0; c < w; c++) {
        vga_set_cursor(x + c, y);
        vga_putc('-', 0x08);
        vga_set_cursor(x + c, y + h - 1);
        vga_putc('-', 0x08);
    }

    /* Tarihce ciz - son w-2 deger */
    int count = w - 2;
    if (count > HISTORY_SIZE) count = HISTORY_SIZE;

    for (int i = 0; i < count; i++) {
        int idx = (history_idx - count + i + HISTORY_SIZE) % HISTORY_SIZE;
        int val = (int)hist[idx];
        int bar_h = (val * (h - 2)) / 100;
        if (bar_h > h - 2) bar_h = h - 2;

        for (int r = 0; r < bar_h; r++) {
            vga_set_cursor(x + 1 + i, y + h - 2 - r);
            vga_putc(0xDB, color);
        }
    }
}

/* Sayi yazdirma yardimcisi */
static void print_num(int x, int y, uint32_t val, uint8_t color) {
    vga_set_cursor(x, y);
    vga_put_dec(val, color);
}

/* Ana ekran cizimi */
static void draw_monitor_screen(void) {
    aegis_theme_t* t = theme_get_current();
    
    tui_draw_frame("SISTEM IZLENCESI");

    /* === CPU PANELI === */
    vga_set_cursor(2, 2);
    vga_puts("[CPU]", t->accent_color);

    vga_set_cursor(2, 3);
    vga_puts("Kullanim: ", t->fg_color);
    /* Renk kodlama: normal=yesil, yuksek=sari, kritik=kirmizi */
    uint8_t cpu_color = 0x0A;
    if (cpu_usage_pct > 80) cpu_color = 0x0C;
    else if (cpu_usage_pct > 50) cpu_color = 0x0E;
    vga_put_dec(cpu_usage_pct, cpu_color);
    vga_putc('%', cpu_color);
    vga_puts("   ", 0x00);

    draw_bar(2, 4, 30, cpu_usage_pct, cpu_color);

    /* CPU tarihce grafigi */
    vga_set_cursor(2, 5);
    vga_puts("Son 30 ornek:", 0x08);
    draw_history_graph(2, 6, 32, 5, cpu_history, cpu_color);

    /* === RAM PANELI === */
    vga_set_cursor(2, 12);
    vga_puts("[RAM / HEAP]", t->accent_color);

    uint32_t heap_pct = get_heap_pct();
    uint32_t heap_used = get_heap_used();
    uint32_t heap_kb = heap_used / 1024;
    uint32_t heap_total_kb = HEAP_SIZE / 1024;

    vga_set_cursor(2, 13);
    vga_puts("Kullanim: ", 0x07);
    vga_put_dec(heap_kb, 0x0F);
    vga_puts(" KB / ", 0x07);
    vga_put_dec(heap_total_kb, 0x0F);
    vga_puts(" KB (", 0x07);
    uint8_t ram_color = 0x0A;
    if (heap_pct > 80) ram_color = 0x0C;
    else if (heap_pct > 50) ram_color = 0x0E;
    vga_put_dec(heap_pct, ram_color);
    vga_puts("%)   ", ram_color);

    draw_bar(2, 14, 30, heap_pct, ram_color);

    /* RAM tarihce grafigi */
    vga_set_cursor(2, 15);
    vga_puts("Son 30 ornek:", 0x08);
    draw_history_graph(2, 16, 32, 5, ram_history, ram_color);

    /* === GPU PANELI === */
    vga_set_cursor(40, 2);
    vga_puts("[GPU / GORUNTU DENETLEYICISI]", t->accent_color);

    vga_set_cursor(40, 3);
    if (gpu_found) {
        vga_puts("Durum: ", 0x07);
        vga_puts("Tespit edildi", 0x0A);
        vga_set_cursor(40, 4);
        vga_puts("Vendor ID: ", 0x07);
        vga_put_hex(gpu_vendor, 0x0F);
        vga_set_cursor(40, 5);
        vga_puts("Device ID: ", 0x07);
        vga_put_hex(gpu_device, 0x0F);
        vga_set_cursor(40, 6);
        vga_puts("Cozunurluk: ", 0x07);
        vga_put_dec(vga_get_width(), 0x0F);
        vga_putc('x', 0x07);
        vga_put_dec(vga_get_height(), 0x0F);
        vga_set_cursor(40, 7);
        vga_puts("Kullanim: ", 0x07);
        vga_puts("Veri yok (surucu gerekli)", 0x08);
    } else {
        vga_puts("Durum: ", 0x07);
        vga_puts("Ayrık GPU bulunamadı", 0x08);
        vga_set_cursor(40, 4);
        vga_puts("Framebuffer: ", 0x07);
        vga_put_dec(vga_get_width(), 0x0F);
        vga_putc('x', 0x07);
        vga_put_dec(vga_get_height(), 0x0F);
    }

    /* === SISTEM BILGI PANELI === */
    vga_set_cursor(40, 9);
    vga_puts("[SISTEM BILGISI]", t->accent_color);

    vga_set_cursor(40, 10);
    vga_puts("Platform  : AegisOS v4.0 x86", 0x07);
    vga_set_cursor(40, 11);
    vga_puts("Cekirdek  : Mikro cekirdek", 0x07);
    vga_set_cursor(40, 12);
    vga_puts("Zamanlayici: PIT 1000Hz (1ms)", 0x07);
    vga_set_cursor(40, 13);
    vga_puts("Bellek    : Bump Allocator", 0x07);
    vga_set_cursor(40, 14);
    vga_puts("Uptime    : ", 0x07);
    uint64_t ms = pit_get_millis();
    uint32_t secs = (uint32_t)(ms / 1000);
    uint32_t mins = secs / 60;
    uint32_t hours = mins / 60;
    vga_put_dec(hours, 0x0F);
    vga_putc(':', 0x07);
    if ((mins % 60) < 10) vga_putc('0', 0x0F);
    vga_put_dec(mins % 60, 0x0F);
    vga_putc(':', 0x07);
    if ((secs % 60) < 10) vga_putc('0', 0x0F);
    vga_put_dec(secs % 60, 0x0F);
    vga_puts("    ", 0x00);

    /* === ALT BILGI === */
    tui_draw_bottom_bar("ESC:Cikis  F5/R:Yenile  F1:Kapat  F2:Restart  F3:Oturum Kapat");

    vga_swap_buffers();
}

/* ============================================================
   ANA UYGULAMA DONGUSU
   ============================================================ */
void app_system_monitor(void) {
    /* Baslangic */
    find_gpu_info();

    /* Tarihceyi temizle */
    for (int i = 0; i < HISTORY_SIZE; i++) {
        cpu_history[i] = 0;
        ram_history[i] = 0;
    }
    history_idx = 0;

    prev_idle = idle_ticks_total;
    prev_total = pit_get_ticks();

    /* Input buffer temizle */
    while (input_available()) input_get();

    uint64_t last_update = pit_get_millis();

    /* Ilk cizim */
    update_cpu_usage();
    update_history();
    draw_monitor_screen();

    while (1) {
        uint64_t now = pit_get_millis();

        /* Her 2 saniyede guncelle */
        if (now - last_update >= 2000) {
            last_update = now;
            update_cpu_usage();
            update_history();
            draw_monitor_screen();
        }

        /* Input kontrol */
        if (input_available()) {
            key_event_t ev = input_get();
            if (!ev.pressed) continue;
            int k = ev.key;

            if (k == 27) { /* ESC - Cikis */
                vga_clear(0x00);
                vga_swap_buffers();
                return;
            }
            if (k == KEY_F5 || k == 'r' || k == 'R') { /* Manuel yenile */
                update_cpu_usage();
                update_history();
                draw_monitor_screen();
            }
            /* Guc islemleri - ileri seviye (power.c entegrasyonu) */
            if (k == KEY_F1) {
                /* Kapatma */
                extern void power_shutdown(void);
                power_shutdown();
            }
            if (k == KEY_F2) {
                /* Yeniden baslat */
                extern void power_reboot(void);
                power_reboot();
            }
            if (k == KEY_F3) {
                /* Oturum kapat */
                extern void session_request_logout(void);
                session_request_logout();
                vga_clear(0x00);
                vga_swap_buffers();
                return;
            }
        }

        /* Kucuk bekleme */
        for (volatile int i = 0; i < 5000; i++)
            __asm__ volatile("pause");
        vga_swap_buffers();
    }
}

/* API fonksiyonlari */
uint32_t sysmon_get_cpu_usage(void)  { return cpu_usage_pct; }
uint32_t sysmon_get_heap_used(void)  { return heap_ptr; }
uint32_t sysmon_get_heap_total(void) { return HEAP_SIZE; }
