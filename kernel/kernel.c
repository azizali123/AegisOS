/* ============================================================
   kernel.c - AegisOS v4.0 Ana Kernel
   Multiboot 2 / UEFI uyumlu
   Grafiksel preload ekrani + ilerleme cubugu
   ============================================================ */
#include "include/vga.h"
#include "include/keyboard.h"
#include "include/mouse.h"
#include "include/audio.h"
#include "include/pit.h"
#include "include/idt.h"
#include "include/memory.h"
#include "include/filesystem.h"
#include "include/rtc.h"
#include "include/pci.h"
#include "include/usb.h"
#include "include/kernel.h"
#include "include/power.h"
#include "include/session.h"
#include "include/network.h"
#include <stdint.h>

extern void shell_run(void);

/* Idle sayaclari - CPU izleme icin */
volatile uint64_t idle_ticks_total  = 0;
volatile uint64_t busy_ticks_total  = 0;
static   uint64_t last_idle_update  = 0;

/* Os_idle_hook: CPU bekleme anlarinda ekrani guncelle ve gucu koru
   v4.0: HLT tabanli guc yonetimi — CPU isiniyor sorununu cozer.
   HLT komutu CPU'yu bir sonraki kesmeye kadar (IRQ) tamamen durdurur.
   Bu, mesgul bekleme (busy-wait) yerine donanim seviyesinde guc tasarrufu saglar. */
void os_idle_hook(void) {
    idle_ticks_total++;
    vga_swap_buffers();
    /* HLT komutu: CPU'yu sonraki IRQ'ya kadar durdurur.
       Bu tek komut, onceki 10000 iterasyonluk busy-wait dongusunun
       yerini alir ve CPU sicakligini dramatik olarak dusurur. */
    __asm__ volatile("hlt");
}

/* ---- Preload Ekrani ---- */
#define BOOT_STEPS 9
static const char *boot_step_names[BOOT_STEPS] = {
    "Bellek yoneticisi baslatiliyor...",
    "IDT + PIC yeniden hazirlaniyor...",
    "PIT zamanlayici baslatiliyor...",
    "Dosya sistemi kuruluyor...",
    "Klavye surucusu yukleniyor...",
    "Fare surucusu yukleniyor...",
    "Ses sistemi hazirlaniyor...",
    "PCI / USB taramasi yapiliyor...",
    "Ag suruculeri yukleniyor..."
};

/* Agirlikli ilerleme (toplam = 100) */
static const int boot_step_weights[BOOT_STEPS] = {
    10, 10, 5, 10, 10, 10, 10, 12, 8
};

static int boot_paused = 0;

/* Preload ekranini ciz */
static void draw_preload_screen(int progress, const char *status_msg) {
    int scr_w = (int)vga_get_width();
    int scr_h = (int)vga_get_height();
    int cols = scr_w / 8;

    vga_clear(0x00);

    /* === LOGO === */
    int logo_y = 8;
    vga_set_cursor(cols/2 - 15, logo_y);
    vga_puts("==============================", 0x09);
    vga_set_cursor(cols/2 - 15, logo_y+1);
    vga_puts("||                          ||", 0x09);
    vga_set_cursor(cols/2 - 15, logo_y+2);
    vga_puts("||   A E G I S  O S  v4.0   ||", 0x0B);
    vga_set_cursor(cols/2 - 15, logo_y+3);
    vga_puts("||   UEFI / x86 Platform    ||", 0x0B);
    vga_set_cursor(cols/2 - 15, logo_y+4);
    vga_puts("||                          ||", 0x09);
    vga_set_cursor(cols/2 - 15, logo_y+5);
    vga_puts("==============================", 0x09);

    /* === ILERLEME CUBUGU === */
    int bar_y = logo_y + 8;
    int bar_w = 50;
    int bar_x = cols/2 - bar_w/2;
    int filled = (progress * bar_w) / 100;
    if (filled > bar_w) filled = bar_w;

    vga_set_cursor(bar_x - 1, bar_y);
    vga_putc('[', 0x0F);
    for (int i = 0; i < bar_w; i++) {
        if (i < filled)
            vga_putc(0xDB, 0x0A);  /* Yesil dolu blok */
        else
            vga_putc(0xB0, 0x08);  /* Koyu gri bos blok */
    }
    vga_putc(']', 0x0F);

    /* Yuzde */
    vga_set_cursor(bar_x + bar_w + 2, bar_y);
    if (progress < 10) {
        vga_putc(' ', 0x0F);
        vga_putc(' ', 0x0F);
    } else if (progress < 100) {
        vga_putc(' ', 0x0F);
    }
    vga_putc((char)('0' + (progress / 100) % 10), 0x0E);
    if (progress >= 10)
        vga_putc((char)('0' + (progress / 10) % 10), 0x0E);
    vga_putc((char)('0' + progress % 10), 0x0E);
    vga_putc('%', 0x0E);

    /* Yuzde duzelt: sadece gercek rakami goster */
    vga_set_cursor(bar_x + bar_w + 2, bar_y);
    char pct_buf[6];
    int pi = 0;
    if (progress >= 100) { pct_buf[pi++] = '1'; pct_buf[pi++] = '0'; pct_buf[pi++] = '0'; }
    else if (progress >= 10) { pct_buf[pi++] = (char)('0' + progress/10); pct_buf[pi++] = (char)('0' + progress%10); }
    else { pct_buf[pi++] = (char)('0' + progress); }
    pct_buf[pi++] = '%';
    pct_buf[pi] = 0;
    /* Temizle */
    vga_puts("     ", 0x00);
    vga_set_cursor(bar_x + bar_w + 2, bar_y);
    vga_puts(pct_buf, 0x0E);

    /* === DURUM MESAJI === */
    vga_set_cursor(cols/2 - 25, bar_y + 2);
    for (int i = 0; i < 50; i++) vga_putc(' ', 0x00);
    vga_set_cursor(cols/2 - 25, bar_y + 2);
    if (status_msg)
        vga_puts(status_msg, 0x07);

    /* Pause mesaji */
    if (boot_paused) {
        vga_set_cursor(cols/2 - 28, bar_y + 4);
        vga_puts("Boot duraklatildi - devam icin herhangi bir tusa basin", 0x0C);
    } else {
        vga_set_cursor(cols/2 - 26, bar_y + 4);
        vga_puts("Herhangi bir tusa basarak boot'u duraklatabilirsiniz", 0x08);
    }

    vga_swap_buffers();
}

/* Onceki adimlarin toplam agirligini hesapla */
static int calc_progress(int step_done) {
    int total = 0;
    for (int i = 0; i < step_done && i < BOOT_STEPS; i++)
        total += boot_step_weights[i];
    /* Son adimdan sonra kalan %15 (RTC + shell gecis) */
    if (step_done >= BOOT_STEPS) total = 100;
    return total;
}

/* Boot sirasinda tus basimi kontrol et (pause/resume) */
static void boot_check_pause(void) {
    if (input_available()) {
        key_event_t ev = input_get();
        if (ev.pressed) {
            boot_paused = !boot_paused;
            while (boot_paused) {
                draw_preload_screen(calc_progress(0), "Boot duraklatildi...");
                if (input_available()) {
                    ev = input_get();
                    if (ev.pressed) {
                        boot_paused = 0;
                        break;
                    }
                }
                for (volatile int i = 0; i < 50000; i++)
                    __asm__ volatile("pause");
            }
        }
    }
}

/* ---- Kernel panik ---- */
void kernel_panic(const char *msg) {
    __asm__ volatile("cli");
    vga_clear(0x44); /* Kirmizi arka plan */
    vga_swap_buffers();
    vga_puts("\n\n  *** KERNEL PANIC ***\n", 0x4F);
    vga_puts("  ", 0x4F);
    vga_puts(msg, 0x4F);
    vga_puts("\n\n  Sistem durduruldu. Yeniden baslatiniz.\n", 0x4F);
    vga_swap_buffers();
    while (1) __asm__ volatile("hlt");
}

/* ============================================================
   Kernel Giris Noktasi — Multiboot2
   eax = 0x36d76289 (Multiboot2 magic)
   ebx = Multiboot2 bilgi yapisi adresi
   ============================================================ */
void kernel_main(uint32_t magic, uint32_t addr) {
    int step = 0;

    /* 1) Grafik modu baslat (Multiboot2 framebuffer etiketi) */
    if (magic == 0x36d76289)
        vga_init_graphics((void *)(uintptr_t)addr);
    else
        vga_init_graphics(NULL); /* Metin moduna gec */

    draw_preload_screen(0, "AegisOS baslatiliyor...");

    /* 2) Bellek */
    draw_preload_screen(calc_progress(step), boot_step_names[step]);
    memory_init();
    step++;

    /* 3) IDT + PIC */
    draw_preload_screen(calc_progress(step), boot_step_names[step]);
    idt_init();
    step++;

    /* 4) PIT (timer) */
    draw_preload_screen(calc_progress(step), boot_step_names[step]);
    pit_init(1000);   /* 1000 Hz = 1ms tick */
    step++;

    /* 5) Dosya sistemi */
    draw_preload_screen(calc_progress(step), boot_step_names[step]);
    fs_init();
    step++;

    /* 6) Klavye - bundan sonra pause kontrol edilebilir */
    draw_preload_screen(calc_progress(step), boot_step_names[step]);
    keyboard_init();
    step++;

    /* Artik tus kontrol edilebilir */
    boot_check_pause();

    /* 7) Fare */
    draw_preload_screen(calc_progress(step), boot_step_names[step]);
    mouse_init();
    step++;
    boot_check_pause();

    /* 8) Ses */
    draw_preload_screen(calc_progress(step), boot_step_names[step]);
    audio_init();
    step++;
    boot_check_pause();

    /* 9) PCI / USB */
    draw_preload_screen(calc_progress(step), boot_step_names[step]);
    usb_init();
    pci_init();
    step++;
    boot_check_pause();

    /* 10) Ag suruculeri */
    draw_preload_screen(calc_progress(step), boot_step_names[step]);
    network_init();
    network_detect_nic();
    step++;
    boot_check_pause();

    /* Baslangic sesi */
    audio_play_startup_sound();

    /* Son ilerleme: RTC ve Shell gecisi */
    draw_preload_screen(95, "Sistem saati okunuyor...");
    /* Kisa gecikme */
    for (volatile int i = 0; i < 500000; i++) __asm__ volatile("pause");

    draw_preload_screen(100, "Tum suruculer yuklendi. Sistem hazir!");
    for (volatile int i = 0; i < 1000000; i++) __asm__ volatile("pause");

    /* Oturum sistemi */
    session_init();

    /* Oturum ekrani goster, ardindan shell */
session_start:
    if (session_login_screen()) {
        /* Shell */
        shell_run();

        /* Shell'den logout ile donulduyse */
        if (session_is_logout_requested()) {
            session_logout();
            goto session_start;
        }
    }

    /* Shell cikarsa */
    while (1) {
        vga_swap_buffers();
        __asm__ volatile("hlt");
    }
}
