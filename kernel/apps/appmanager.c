#include "../include/filesystem.h"
#include "../include/input.h"
#include "../include/string.h"
#include "../include/vga.h"
#include "../include/tui.h"
#include <stdint.h>

extern int current_dir_id;

/* Uygulama tanımlamaları */
typedef struct {
  const char *name;     /* Görünen isim */
  const char *filename; /* Dosya adı (örnek: TETRIS.APP) */
  const char *folder;   /* Klasör (SISTEM veya OYUNLAR) */
  int installed;        /* Yüklü mü? */
} AppEntry;

#define APP_COUNT 12
static AppEntry app_list[APP_COUNT] = {
    {"Saat", "SAAT.APP", "SISTEM", 0},
    {"Metin Editoru", "EDIT.APP", "SISTEM", 0},
    {"Hesap Makinesi", "HESAP.APP", "SISTEM", 0},
    {"Dosya Gezgini", "GEZGIN.APP", "SISTEM", 0},
    {"Sistem Izlencesi", "SYSMON.APP", "SISTEM", 0},
    {"Tetris", "TETRIS.APP", "OYUNLAR", 0},
    {"Pentomino", "PENTO.APP", "OYUNLAR", 0},
    {"Yilan Oyunu", "SNAKE.APP", "OYUNLAR", 0},
    {"Pong", "PONG.APP", "OYUNLAR", 0},
    {"Pacman", "PACMAN.APP", "OYUNLAR", 0},
    {"XOX", "XOX.APP", "OYUNLAR", 0},
    {"Mayin Tarlasi", "MINE.APP", "OYUNLAR", 0},
};

static int mgr_selected = 0;

/* Bir uygulamanın yüklü olup olmadığını kontrol et */
static void check_installed_status(void) {
  for (int i = 0; i < APP_COUNT; i++) {
    int old_dir = current_dir_id;

    /* Kök dizine git */
    current_dir_id = -1;

    /* Hedef klasöre git */
    fs_change_dir(app_list[i].folder);

    /* Dosya listesini oku */
    int count = 0;
    char names[64][64];
    fs_get_files(&count, names, 0);

    app_list[i].installed = 0;
    for (int j = 0; j < count; j++) {
      if (strcmp(names[j], app_list[i].filename) == 0) {
        app_list[i].installed = 1;
        break;
      }
    }

    current_dir_id = old_dir;
  }
}

/* Uygulama yükle */
static void install_app(int idx) {
  int old_dir = current_dir_id;

  /* Kök dizine git */
  current_dir_id = -1;

  /* Klasörü oluştur (varsa sorun yok) */
  mkdir(app_list[idx].folder);

  /* Klasöre gir */
  fs_change_dir(app_list[idx].folder);

  /* Uygulama dosyasını oluştur */
  File *f = fopen(app_list[idx].filename, "w");
  if (f)
    fclose(f);

  current_dir_id = old_dir;
  app_list[idx].installed = 1;
}

/* Uygulama kaldır */
static void uninstall_app(int idx) {
  int old_dir = current_dir_id;

  /* Kök dizine git */
  current_dir_id = -1;

  /* Klasöre gir */
  fs_change_dir(app_list[idx].folder);

  /* Dosyayı sil */
  fremove(app_list[idx].filename);

  current_dir_id = old_dir;
  app_list[idx].installed = 0;
}

static void draw_mgr_frame(void) {
  aegis_theme_t* t = theme_get_current();
  tui_draw_frame("AEGIS UYGULAMA MERKEZI");

  /* Kolon başlıkları */
  vga_set_cursor(5, 4);
  vga_puts("  UYGULAMA ADI           KLASOR       DURUM         ISLEM", t->fg_color);

  tui_draw_bottom_bar("YON OKLARI: SEC    ENTER: YUKLE/KALDIR    T: TUMUNU YUKLE    ESC: CIKIS");
}

static void draw_app_list(void) {
  aegis_theme_t* t = theme_get_current();
  for (int i = 0; i < APP_COUNT; i++) {
    int row = 5 + i;
    uint8_t color = (i == mgr_selected) ? t->accent_color : t->fg_color;

    /* Satırı temizle */
    for (int c = 3; c < 77; c++) {
      vga_set_cursor(c, row);
      vga_putc(' ', t->bg_color);
    }

    /* Numara */
    char num[4];
    int n = i + 1;
    if (n >= 10) {
      num[0] = '0' + (n / 10);
      num[1] = '0' + (n % 10);
      num[2] = '.';
      num[3] = 0;
    } else {
      num[0] = '0' + n;
      num[1] = '.';
      num[2] = 0;
    }
    vga_set_cursor(5, row);
    vga_puts(num, color);

    /* Uygulama adı */
    vga_set_cursor(8, row);
    vga_puts(app_list[i].name, color);

    /* Klasör */
    vga_set_cursor(30, row);
    vga_puts(app_list[i].folder, color);

    /* Durum */
    vga_set_cursor(43, row);
    if (app_list[i].installed) {
      vga_puts("[YUKLU]", t->accent_color);
    } else {
      vga_puts("[YOK]  ", t->border_color);
    }

    /* İşlem */
    vga_set_cursor(55, row);
    if (app_list[i].installed) {
      vga_puts("ENTER->Kaldir", color);
    } else {
      vga_puts("ENTER->Yukle ", color);
    }
  }
}

void app_manager(void) {
  int initial_dir = current_dir_id;
  mgr_selected = 0;
  check_installed_status();

  int needs_redraw = 1;

  while (1) {
    if (needs_redraw) {
      draw_mgr_frame();
      draw_app_list();
      needs_redraw = 0;
    }

    key_event_t ev = input_get();
    if (!ev.pressed) continue;
    int k = ev.key;

    if (k == 27) { /* ESC */
      current_dir_id = initial_dir;
      return;
    }

    if (k == KEY_UP) {
      if (mgr_selected > 0) {
        mgr_selected--;
        draw_app_list();
      }
    }

    if (k == KEY_DOWN) {
      if (mgr_selected < APP_COUNT - 1) {
        mgr_selected++;
        draw_app_list();
      }
    }

    if (k == '\n') { /* ENTER: Yükle veya Kaldır */
      if (app_list[mgr_selected].installed) {
        uninstall_app(mgr_selected);
      } else {
        install_app(mgr_selected);
      }
      draw_app_list();
    }

    if (k == 't' || k == 'T') { /* Tümünü yükle */
      for (int i = 0; i < APP_COUNT; i++) {
        if (!app_list[i].installed) {
          install_app(i);
        }
      }
      draw_app_list();
    }
  }
}
