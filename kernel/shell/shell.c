/* ============================================================
   shell.c - AegisOS v4.0 Komut Kabugu
   RAM-FS uyumlu, yeni fs_* API kullanir
   Guncellenmis: sysmon, power, session, komut kurallari
   ============================================================ */

#include "../include/filesystem.h"
#include "../include/keyboard.h"
#include "../include/string.h"
#include "../include/vga.h"
#include "../include/rtc.h"
#include "../include/memory.h"
#include "../include/audio.h"
#include "../include/power.h"
#include "../include/session.h"
#include "../include/system_monitor.h"
#include "../include/network.h"
#include <stdint.h>

/* Uygulama prototipleri */
void game_tetris(void);
void game_snake(void);
void game_xox(void);
void game_minesweeper(void);
void game_pong(void);
void game_pacman(void);
void game_pentomino(void);
void app_calculator(void);
void app_editor(const char *filename);
void app_clock(void);
void app_explorer(void);
void app_manager(void);
void display_settings_open(void);
void app_firewall(void);

/* ============================================================
   Komut Tamponu
   ============================================================ */
#define CMD_BUF 256
static char cmd_buf[CMD_BUF];
static int  cmd_len = 0;

/* Komut satiri girisi — echo ile */
static void read_line(void) {
    cmd_len = 0;
    cmd_buf[0] = 0;
    while (1) {
        int k = keyboard_read_key();
        if (k == '\n' || k == '\r') { vga_putc('\n', 0x07); break; }
        if (k == '\b') {
            if (cmd_len > 0) {
                cmd_len--; cmd_buf[cmd_len] = 0;
                vga_putc('\b', 0x07);
            }
            continue;
        }
        if (k >= 32 && k <= 255 && cmd_len < CMD_BUF - 1) {
            cmd_buf[cmd_len++] = (char)k;
            cmd_buf[cmd_len]   = 0;
            vga_putc((char)k, 0x07);
        }
    }
}

/* Turkce karakterleri ASCII'ye normalize et */
static void normalize(const char *src, char *dst, int max) {
    int i;
    for (i = 0; i < max - 1 && src[i]; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c >= 'A' && c <= 'Z')     dst[i] = (char)(c + 32);
        else if (c == 133 || c == 134) dst[i] = 'i'; /* ı, İ */
        else if (c == 129 || c == 139) dst[i] = 'g'; /* ğ, Ğ */
        else if (c == 132 || c == 137) dst[i] = 'u'; /* ü, Ü */
        else if (c == 130 || c == 136) dst[i] = 's'; /* ş, Ş */
        else if (c == 131 || c == 138) dst[i] = 'o'; /* ö, Ö */
        else if (c == 128 || c == 135) dst[i] = 'c'; /* ç, Ç */
        else dst[i] = (char)c;
    }
    dst[i] = 0;
}

/* ============================================================
   Durum Cubugu (ust sag — saat)
   ============================================================ */
static void update_clock_display(void) {
    static uint32_t last_sec = 0xFF;
    uint8_t d,mo,y,h,m,s;
    rtc_get_time(&d,&mo,&y,&h,&m,&s);
    if (s == last_sec) return;
    last_sec = s;
    int cx, cy; vga_get_cursor(&cx, &cy);
    vga_set_cursor(110, 0);
    char buf[9]; /* HH:MM:SS */
    buf[0]=(char)('0'+h/10); buf[1]=(char)('0'+h%10); buf[2]=':';
    buf[3]=(char)('0'+m/10); buf[4]=(char)('0'+m%10); buf[5]=':';
    buf[6]=(char)('0'+s/10); buf[7]=(char)('0'+s%10); buf[8]=0;
    vga_puts(buf, 0x0E);
    vga_set_cursor(cx, cy);
}

/* ============================================================
   Komut Isleyiciler
   ============================================================ */
static void cmd_ls(void) {
    char names[FS_MAX_FILES][FS_MAX_FILENAME];
    int n = fs_list(names, FS_MAX_FILES);
    if (n == 0) { vga_puts("  (dosya yok)\n", 0x08); return; }
    for (int i = 0; i < n; i++) {
        uint32_t sz = fs_get_size(names[i]);
        vga_puts("  ", 0x07);
        vga_puts(names[i], 0x0F);
        vga_puts("  (", 0x07); vga_put_dec(sz, 0x0B); vga_puts(" byte)\n", 0x07);
    }
}

static void cmd_cat(const char *fname) {
    static uint8_t buf[FS_MAX_FILESIZE];
    int n = fs_read(fname, buf, sizeof(buf) - 1);
    if (n < 0) { vga_puts("Dosya bulunamadi: ", 0x0C); vga_puts(fname, 0x0C); vga_newline(); return; }
    buf[n] = 0;
    vga_puts((const char *)buf, 0x0F);
    vga_newline();
}

static void cmd_touch(const char *fname) {
    if (fs_create(fname) >= 0) { vga_puts("Olusturuldu: ", 0x0A); vga_puts(fname, 0x0F); vga_newline(); }
    else vga_puts("HATA: Olusturulamadi.\n", 0x0C);
}

/* Dosya silme - korumali alan kontrolu ile */
static void cmd_rm(const char *fname) {
    if (!fname || fname[0] == 0) {
        vga_puts("Kullanim: rm/del/sil <dosya_adi>\n", 0x0E);
        return;
    }
    /* Korumali dosya kontrolu */
    if (strcmp(fname, "aegis_core.bin") == 0 ||
        strcmp(fname, "boot.S") == 0 ||
        strcmp(fname, "grub.cfg") == 0) {
        vga_puts("HATA: Korumali sistem dosyasi silinemez: ", 0x0C);
        vga_puts(fname, 0x0C);
        vga_newline();
        return;
    }
    if (fs_delete(fname) == 0) {
        vga_puts("Silindi: ", 0x0A);
        vga_puts(fname, 0x0F);
        vga_newline();
    } else {
        vga_puts("HATA: Dosya bulunamadi: ", 0x0C);
        vga_puts(fname, 0x0C);
        vga_newline();
    }
}

static void cmd_cd(const char *arg) {
    if (!arg || arg[0] == 0) {
        vga_puts("  Mevcut konum: / (kok dizin)\n", 0x07);
        vga_puts("  AegisOS RAM-FS su an duz (flat) yapi kullanir.\n", 0x0E);
        vga_puts("  'cd ..' ile ust dizine, 'cd <klasor>' ile alt dizine gecebilirsiniz.\n", 0x08);
        return;
    }
    extern int current_dir_id;
    if (strcmp(arg, "/") == 0) {
        current_dir_id = -1;
        vga_puts("  Kok dizine donuldu.\n", 0x0A);
    } else if (strcmp(arg, "..") == 0) {
        if (current_dir_id >= 0) {
            fs_change_dir("..");
            vga_puts("  Ust dizine gecildi.\n", 0x0A);
        } else {
            vga_puts("  Zaten kok dizindesiniz.\n", 0x0E);
        }
    } else {
        /* Hedef klasorun var olup olmadigini kontrol et */
        int target = fs_find_dir(arg, current_dir_id);
        if (target >= 0) {
            fs_change_dir(arg);
            vga_puts("  Dizine gecildi: ", 0x0A);
            vga_puts(arg, 0x0F);
            vga_newline();
        } else {
            vga_puts("  HATA: Klasor bulunamadi: ", 0x0C);
            vga_puts(arg, 0x0C);
            vga_newline();
        }
    }
}

static void cmd_make(void) {
    vga_puts("  [DERLE] Derleyici/Kurucu arac zinciri henuz aktif degil.\n", 0x0E);
    vga_puts("  Derlenecek hedef bulunamadi.\n", 0x08);
    vga_puts("  Not: Gercek bir derleyici zinciri baglaninca bu komut aktif olacaktir.\n", 0x08);
}

static void cmd_mem(void) { memory_print_stats(); }

static void cmd_tarih(void) {
    vga_puts("  Tarih/Saat: ", 0x0B);
    rtc_print_time();
}

static void cmd_ses(const char *arg) {
    (void)arg;
    uint32_t freq = 440, dur = 300;
    audio_play_note(freq, dur);
}

static void cmd_about(void) {
    vga_puts("\n", 0x07);
    vga_puts("  +----------------------------------------------+\n", 0x09);
    vga_puts("  |          AEGiS ISLETIM SiSTEMi v4.0         |\n", 0x0B);
    vga_puts("  +----------------------------------------------+\n", 0x09);
    vga_puts("  |  Platform   : UEFI / x86  (32/64-bit)       |\n", 0x0F);
    vga_puts("  |  Cekirdek   : Ozgun yikisiz mikro-cekirdek  |\n", 0x0F);
    vga_puts("  |  Suruculer  : PS/2 Klavye+Fare, PC Speaker  |\n", 0x0F);
    vga_puts("  |             : AC97 Ses, RTC, PIT, xHCI/EHCI |\n", 0x0F);
    vga_puts("  |             : Network, VPN, Firewall        |\n", 0x0F);
    vga_puts("  |  FS         : RAM-FS (IDE yoksa otomatik)   |\n", 0x0F);
    vga_puts("  |  Cizim      : VGA DoubleBuffer + DirtyRect  |\n", 0x0F);
    vga_puts("  |  Oyunlar    : Tetris, Pentomino, Snake,     |\n", 0x0F);
    vga_puts("  |             : Pong, Pacman, XOX, Mayin      |\n", 0x0F);
    vga_puts("  +----------------------------------------------+\n\n", 0x09);
}

static void cmd_help(void) {
    vga_puts("\n", 0x07);
    vga_puts("  KOMUT              ACIKLAMA\n", 0x0B);
    vga_puts("  -------------------------------------------------\n", 0x08);
    const char *cmds[][2] = {
        {"ls / dir",          "Dosyalari listele"},
        {"cd / git / konum",  "Dizin degistir"},
        {"cat [dosya]",       "Dosya icerigini goster"},
        {"touch [dosya]",     "Yeni dosya olustur"},
        {"rm / del / sil",    "Dosya sil"},
        {"edit [dosya]",      "Metin editoru"},
        {"explorer",          "Dosya gezgini"},
        {"tarih / date",      "Tarih/Saat goster"},
        {"mem",               "Bellek kullanimi"},
        {"ses",               "Bip sesi cal"},
        {"sysmon / izle",     "Sistem izlencesi"},
        {"shutdown / kapat",  "Sistemi kapat"},
        {"reboot / ybaslat",  "Yeniden baslat"},
        {"logout / cikis",    "Oturumu kapat"},
        {"tetris",            "Tetris oyunu"},
        {"pentomino",         "Pentomino Challenge"},
        {"snake / yilan",     "Yilan oyunu"},
        {"xox",               "XOX oyunu"},
        {"pong",              "Pong oyunu"},
        {"pacman",            "Pacman oyunu"},
        {"mine / mayin",      "Mayin tarlasi"},
        {"calc / hesap",      "Hesap makinesi"},
        {"clock / saat",      "Dijital saat"},
        {"magaza / yukle",    "Uygulama yoneticisi"},
        {"ekran / display",   "Ekran ayarlari"},
        {"ag tara",           "WiFi aglari tara"},
        {"ag baglan <SSID>",  "Aga baglan"},
        {"ag kes",            "Baglanti kes"},
        {"ag durum",          "Ag durum bilgisi"},
        {"vpn",               "VPN ac/kapat"},
        {"firewall",          "Guvenlik Duvari"},
        {"mac spoof",         "MAC adresini degistir"},
        {"make / derle",      "Derleme komutu"},
        {"clear / temizle",   "Ekrani temizle"},
        {"hakkinda / about",  "Sistem bilgisi"},
        {0,0}
    };
    for (int i = 0; cmds[i][0]; i++) {
        vga_puts("  ", 0x07);
        vga_puts(cmds[i][0], 0x0E);
        int pad = 20 - (int)strlen(cmds[i][0]);
        for (int p = 0; p < pad; p++) vga_putc(' ', 0x07);
        vga_puts(cmds[i][1], 0x07);
        vga_newline();
    }
    vga_newline();
}

/* ============================================================
   Ana Kabuk Dongusu
   ============================================================ */
void shell_run(void) {
    vga_clear(0x00);

    /* Karsilama */
    vga_puts("\n", 0x07);
    vga_puts("  +----------------------------------------------+\n", 0x03);
    vga_puts("  |        AEGiS ISLETIM SiSTEMi v4.0           |\n", 0x0B);
    vga_puts("  +----------------------------------------------+\n", 0x03);
    vga_puts("  Hosgeldiniz! 'help' yazarak komutlara bakabilirsiniz.\n\n", 0x07);

    audio_play_click();

    while (1) {
        update_clock_display();
        vga_puts("AegisOS> ", 0x0A);
        read_line();
        if (cmd_len == 0) continue;

        char nc[CMD_BUF];
        normalize(cmd_buf, nc, CMD_BUF);

        /* Arg ayir */
        char *arg = 0;
        for (int i = 0; nc[i]; i++) {
            if (nc[i] == ' ') { nc[i] = 0; arg = cmd_buf + i + 1; break; }
        }

        /* Dosya islemleri */
        if      (!strcmp(nc,"ls")       || !strcmp(nc,"dir")       || !strcmp(nc,"listele")) cmd_ls();
        else if (!strcmp(nc,"cd")       || !strcmp(nc,"git")       || !strcmp(nc,"konum"))   cmd_cd(arg);
        else if (!strcmp(nc,"cat")      || !strcmp(nc,"oku")       || !strcmp(nc,"goster") || !strcmp(nc,"dosya oku"))  { if(arg) cmd_cat(arg); else vga_puts("Kullanim: cat <dosya>\n", 0x0E); }
        else if (!strcmp(nc,"touch")    || !strcmp(nc,"olustur")   || !strcmp(nc,"dosya olustur"))                            { if(arg) cmd_touch(arg); else vga_puts("Kullanim: touch <dosya>\n", 0x0E); }
        else if (!strcmp(nc,"rm")       || !strcmp(nc,"del")       || !strcmp(nc,"sil")    || !strcmp(nc,"dosya sil"))     cmd_rm(arg);
        else if (!strcmp(nc,"edit")     || !strcmp(nc,"catf")      || !strcmp(nc,"duzenle") || !strcmp(nc,"ac") || !strcmp(nc,"dosya yaz")) { app_editor(arg); vga_clear(0x00); }
        else if (!strcmp(nc,"mkdir")    || !strcmp(nc,"klasor"))                             vga_puts("  Klasor destegi: 'explorer' ile yonetebilirsiniz.\n", 0x0E);
        else if (!strcmp(nc,"make")     || !strcmp(nc,"derle")     || !strcmp(nc,"kur"))     cmd_make();
        else if (!strcmp(nc,"explorer"))                           { app_explorer(); vga_clear(0x00); }
        else if (!strcmp(nc,"tarih")    || !strcmp(nc,"date"))     cmd_tarih();
        else if (!strcmp(nc,"mem"))                                cmd_mem();
        else if (!strcmp(nc,"ses"))                                cmd_ses(arg);
        /* Oyunlar */
        else if (!strcmp(nc,"tetris"))                             { game_tetris();      vga_clear(0x00); }
        else if (!strcmp(nc,"snake")    || !strcmp(nc,"yilan"))    { game_snake();       vga_clear(0x00); }
        else if (!strcmp(nc,"xox"))                                { game_xox();         vga_clear(0x00); }
        else if (!strcmp(nc,"pong"))                               { game_pong();        vga_clear(0x00); }
        else if (!strcmp(nc,"pacman")   || !strcmp(nc,"pakman"))   { game_pacman();      vga_clear(0x00); }
        else if (!strcmp(nc,"mine")     || !strcmp(nc,"mayin"))    { game_minesweeper(); vga_clear(0x00); }
        else if (!strcmp(nc,"pentomino")|| !strcmp(nc,"pento"))    { game_pentomino();   vga_clear(0x00); }
        /* Uygulamalar */
        else if (!strcmp(nc,"calc")     || !strcmp(nc,"hesap"))    { app_calculator();   vga_clear(0x00); }
        else if (!strcmp(nc,"clock")    || !strcmp(nc,"saat"))     { app_clock();        vga_clear(0x00); }
        else if (!strcmp(nc,"magaza")   || !strcmp(nc,"yukle")     || !strcmp(nc,"uygulama yukle"))    { app_manager();      vga_clear(0x00); }
        else if (!strcmp(nc,"sysmon")   || !strcmp(nc,"izle")      || !strcmp(nc,"monitor") || !strcmp(nc,"sistem izleme")) { app_system_monitor(); vga_clear(0x00); }
        /* Guc ve oturum */
        else if (!strcmp(nc,"shutdown") || !strcmp(nc,"kapat")     || !strcmp(nc,"sistem kapat"))    { power_shutdown(); }
        else if (!strcmp(nc,"reboot")   || !strcmp(nc,"ybaslat")   || !strcmp(nc,"yeniden")       || !strcmp(nc,"sistem yenidenbaslat")) { power_reboot(); }
        else if (!strcmp(nc,"logout")   || !strcmp(nc,"cikis")     || !strcmp(nc,"cik")           || !strcmp(nc,"cikis yap")) {
            session_request_logout();
            return;
        }
        /* --- Ag & Network Komutlari (v4.0 network.h API) --- */
        else if (!strcmp(nc,"ag tara")  || !strcmp(nc,"net scan")) {
            wifi_network_t nets[8];
            int n = network_wifi_scan(nets, 8);
            vga_puts("\n  AEGIS AG TARAMASI\n", 0x0B);
            vga_puts("  ------------------------------------------------\n", 0x08);
            for (int i = 0; i < n; i++) {
                vga_puts("  ", 0x07);
                vga_puts(nets[i].encrypted ? "[#] " : "[ ] ", nets[i].encrypted ? 0x0A : 0x0C);
                vga_puts(nets[i].ssid, 0x0F);
                vga_puts("  Sinyal: %", 0x07);
                vga_put_dec((uint32_t)nets[i].signal_strength, 0x0E);
                vga_puts("  CH:", 0x07);
                vga_put_dec((uint32_t)nets[i].channel, 0x0E);
                vga_newline();
            }
            if (n == 0) vga_puts("  Ag bulunamadi.\n", 0x0C);
            vga_newline();
        }
        else if (!strcmp(nc,"ag baglan") || !strcmp(nc,"net connect")) {
            if (arg) {
                vga_puts("  Baglaniyor: ", 0x0B); vga_puts(arg, 0x0F); vga_puts("...\n", 0x0B);
                network_connect(arg, "");
                vga_puts("  Baglanti basarili!\n", 0x0A);
                network_print_status();
            } else {
                vga_puts("  Kullanim: ag baglan <SSID>\n", 0x0E);
            }
        }
        else if (!strcmp(nc,"ag kes") || !strcmp(nc,"net disconnect")) {
            network_disconnect();
            vga_puts("  Baglanti kesildi.\n", 0x0A);
        }
        else if (!strcmp(nc,"ag durum") || !strcmp(nc,"net status")) {
            vga_puts("\n", 0x07);
            network_print_status();
        }
        else if (!strcmp(nc,"vpn")) {
            int st = network_vpn_toggle();
            vga_puts("  VPN: ", 0x0B);
            vga_puts(st ? "AKTIF (WireGuard)\n" : "KAPALI\n", st ? 0x0A : 0x0C);
        }
        else if (!strcmp(nc,"firewall") || !strcmp(nc,"guvenlik duvari") || !strcmp(nc,"ag merkezi")) {
            app_firewall();
            vga_clear(0x00);
        }
        else if (!strcmp(nc,"mac spoof") || !strcmp(nc,"mac degistir")) {
            network_mac_spoof();
            vga_puts("  MAC adresi degistirildi (Anonim Mod)\n", 0x0A);
        }
        /* Genel */
        else if (!strcmp(nc,"ekran")    || !strcmp(nc,"display"))  { display_settings_open(); vga_clear(0x00); }
        else if (!strcmp(nc,"clear")    || !strcmp(nc,"temizle"))  vga_clear(0x00);
        else if (!strcmp(nc,"help")     || !strcmp(nc,"yardim"))   cmd_help();
        else if (!strcmp(nc,"hakkinda") || !strcmp(nc,"about")     || !strcmp(nc,"sys info") || !strcmp(nc,"sistem bilgi"))    cmd_about();
        else {
            vga_puts("Bilinmeyen komut: ", 0x0C);
            vga_puts(cmd_buf, 0x0C);
            vga_puts("  ('help' yazin)\n", 0x08);
        }
    }
}
