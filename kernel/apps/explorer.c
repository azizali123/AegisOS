#include "../include/filesystem.h"
#include "../include/input.h"
#include "../include/string.h"
#include "../include/vga.h"
#include "../include/tui.h"
#include <stdint.h>

void app_editor(const char *filename);
extern void app_clock(void);
extern void app_calculator(void);
extern void game_tetris(void);
extern void game_snake(void);
extern void game_pong(void);
extern void game_pacman(void);
extern void game_xox(void);
extern void game_minesweeper(void);
extern void app_manager(void);
extern void app_system_monitor(void);

#define MAX_FILES 64
static char file_list[MAX_FILES][64];
static int file_is_dir[MAX_FILES];
static int file_count = 0;
static int selected_index = 0;

static char *my_strcpy(char *dest, const char *src) {
  char *ret = dest;
  while (*src)
    *dest++ = *src++;
  *dest = '\0';
  return ret;
}

static char *my_strcat(char *dest, const char *src) {
  char *ret = dest;
  while (*dest)
    dest++;
  while (*src)
    *dest++ = *src++;
  *dest = '\0';
  return ret;
}

static void sort_file_list(void) {
  /* Önce klasörleri, sonra dosyaları sırala */
  for (int i = 0; i < file_count - 1; i++) {
    for (int j = i + 1; j < file_count; j++) {
      /* Klasörler önce gelsin */
      int swap = 0;
      if (!file_is_dir[i] && file_is_dir[j]) {
        swap = 1;
      } else if (file_is_dir[i] == file_is_dir[j]) {
        if (strcmp(file_list[i], file_list[j]) > 0)
          swap = 1;
      }
      if (swap) {
        char temp[64];
        my_strcpy(temp, file_list[i]);
        my_strcpy(file_list[i], file_list[j]);
        my_strcpy(file_list[j], temp);

        int temp_dir = file_is_dir[i];
        file_is_dir[i] = file_is_dir[j];
        file_is_dir[j] = temp_dir;
      }
    }
  }
}

static void scan_root_dir(void) {
  fs_get_files(&file_count, file_list, file_is_dir);
  sort_file_list();
  if (file_count > 0 && selected_index >= file_count) {
    selected_index = file_count - 1;
  }
}

static void draw_box(int x, int y, int w, int h, uint8_t bg_color,
                     const char *title) {
  for (int r = y; r < y + h; r++) {
    for (int c = x; c < x + w; c++) {
      vga_set_cursor(c, r);
      vga_putc(' ', bg_color);
    }
  }

  vga_set_cursor(x, y);
  vga_putc(201, bg_color);
  for (int c = x + 1; c < x + w - 1; c++) {
    vga_set_cursor(c, y);
    vga_putc(205, bg_color);
  }
  vga_set_cursor(x + w - 1, y);
  vga_putc(187, bg_color);

  vga_set_cursor(x, y + h - 1);
  vga_putc(200, bg_color);
  for (int c = x + 1; c < x + w - 1; c++) {
    vga_set_cursor(c, y + h - 1);
    vga_putc(205, bg_color);
  }
  vga_set_cursor(x + w - 1, y + h - 1);
  vga_putc(188, bg_color);

  for (int r = y + 1; r < y + h - 1; r++) {
    vga_set_cursor(x, r);
    vga_putc(186, bg_color);
    vga_set_cursor(x + w - 1, r);
    vga_putc(186, bg_color);
  }

  if (title) {
    int title_len = strlen(title);
    vga_set_cursor(x + (w - title_len) / 2, y);
    vga_puts(title, bg_color);
  }
}

static int popup_confirm(const char *msg) {
  draw_box(15, 8, 50, 6, 0x4F, " SISTEM UYARISI ");
  vga_set_cursor(17, 10);
  vga_puts(msg, 0x4F);
  vga_set_cursor(17, 11);
  vga_puts("Emin misiniz? (E: Evet, H: Hayir)", 0x4F);
  while (1) {
    key_event_t ev = input_get();
    if (!ev.pressed) continue;
    int k = ev.key;
    if (k == 'e' || k == 'E')
      return 1;
    if (k == 'h' || k == 'H' || k == 27)
      return 0;
  }
}

static int popup_input(const char *title, char *out_buf, int max_len) {
  draw_box(15, 8, 50, 6, 0x4F, title);
  vga_set_cursor(17, 10);
  vga_puts("Isim girip ENTER'a basin, veya iptal icin ESC:", 0x4F);

  int len = 0;
  out_buf[0] = 0;

  while (1) {
    vga_set_cursor(17, 11);
    for (int i = 0; i < 46; i++)
      vga_putc(' ', 0x4F);

    vga_set_cursor(17, 11);
    vga_puts("> ", 0x4E);
    vga_puts(out_buf, 0x4F);

    key_event_t ev = input_get();
    if (!ev.pressed) continue;
    int k = ev.key;
    if (k == 27) {
      return 0;
    } else if (k == '\n') {
      if (len > 0)
        return 1;
    } else if (k == '\b') {
      if (len > 0) {
        len--;
        out_buf[len] = 0;
      }
    } else if (k >= 32 && k <= 126) {
      if (len < max_len - 1) {
        out_buf[len++] = (char)k;
        out_buf[len] = 0;
      }
    }
  }
}

static void draw_explorer_frame(void) {
  tui_draw_frame("AEGIS DOSYA YONETICISI v4");
  aegis_theme_t* t = theme_get_current();

  /* Mevcut dizin bilgisi */
  vga_set_cursor(30, 2);
  vga_puts(current_dir_id < 0 ? "[/]" : "[Alt Dizin]", t->accent_color);

  tui_draw_bottom_bar("ESC:CIK  N:YENI DOSYA  K:YENI KLASOR  ENTER:AC  D:SIL");
}

static void print_list_item(int x, int y, const char *text, uint8_t color, uint8_t bg_color) {
  vga_set_cursor(x, y);
  vga_puts(text, color);
  int cur_x = x + strlen(text);
  while (cur_x < 77) {
    vga_set_cursor(cur_x, y);
    vga_putc(' ', bg_color);
    cur_x++;
  }
}

void app_explorer(void) {
  /* Kök dizine dön */
  current_dir_id = -1;
  scan_root_dir();
  selected_index = 0;

  int needs_redraw = 1;

  while (1) {
    if (needs_redraw) {
      aegis_theme_t* t = theme_get_current();
      draw_explorer_frame();

      int row = 0;

      /* ".. (Ust Dizin)" girdisi ekle (kök dizin değilsek) */
      if (current_dir_id >= 0) {
        uint8_t color = (selected_index == 0) ? t->accent_color : t->fg_color;
        print_list_item(7, 4, ".. <UST DIZIN>", color, t->bg_color);
        row = 1;
      }

      for (int i = 0; i < file_count && (row + i) < 18; i++) {
        int display_idx = (current_dir_id >= 0) ? i + 1 : i;
        uint8_t color = (display_idx == selected_index) ? t->accent_color : t->fg_color;
        char disp_name[64];
        if (file_is_dir[i]) {
          my_strcpy(disp_name, "[DIR] ");
          my_strcat(disp_name, file_list[i]);
        } else {
          char num_str[8];
          int n = i + 1, j = 0;
          if (n == 0)
            num_str[j++] = '0';
          else {
            while (n > 0) {
              num_str[j++] = '0' + (n % 10);
              n /= 10;
            }
            for (int k_idx = 0; k_idx < j / 2; k_idx++) {
              char t = num_str[k_idx];
              num_str[k_idx] = num_str[j - 1 - k_idx];
              num_str[j - 1 - k_idx] = t;
            }
          }
          num_str[j] = 0;
          my_strcpy(disp_name, num_str);
          my_strcat(disp_name, ". ");
          my_strcat(disp_name, file_list[i]);
        }
        print_list_item(7, 4 + row + i, disp_name, color, t->bg_color);
      }

      int total_items = file_count + (current_dir_id >= 0 ? 1 : 0);
      if (total_items == 0) {
        print_list_item(7, 4, "(Bos Dizin)", t->fg_color, t->bg_color);
      }

      needs_redraw = 0;
    }

    key_event_t ev = input_get();
    if (!ev.pressed) continue;
    int k = ev.key;
    int total_items = file_count + (current_dir_id >= 0 ? 1 : 0);

    if (k == 27) { /* ESC */
      current_dir_id = -1;
      return;
    }
    if (k == 'b' || k == 'B') { /* Geri (üst dizin) */
      if (current_dir_id >= 0) {
        fs_change_dir("..");
        scan_root_dir();
        selected_index = 0;
        needs_redraw = 1;
      }
    }
    if (k == 'r' || k == 'R') { /* Yenile */
      current_dir_id = -1;
      scan_root_dir();
      selected_index = 0;
      needs_redraw = 1;
    }
    if (k == 'n' || k == 'N') { /* Yeni dosya */
      char new_file[15];
      if (popup_input(" YENI DOSYA OLUSTUR ", new_file, 13)) {
        File *f = fopen(new_file, "w");
        if (f)
          fclose(f);
        scan_root_dir();
      }
      needs_redraw = 1;
    }
    if (k == 'k' || k == 'K') { /* Yeni klasör */
      char new_dir[15];
      if (popup_input(" YENI KLASOR OLUSTUR ", new_dir, 13)) {
        mkdir(new_dir);
        scan_root_dir();
      }
      needs_redraw = 1;
    }
    if (k == KEY_UP) {
      if (selected_index > 0) {
        selected_index--;
        needs_redraw = 1;
      }
    }
    if (k == KEY_DOWN) {
      if (selected_index < total_items - 1) {
        selected_index++;
        needs_redraw = 1;
      }
    }
    if (k == 'd' || k == 'D') { /* Sil */
      /* ".." girdisi silinemez */
      int file_idx = (current_dir_id >= 0) ? selected_index - 1 : selected_index;
      if (file_idx >= 0 && file_idx < file_count) {
        char msg[64];
        my_strcpy(msg, file_list[file_idx]);
        my_strcat(msg, " dosyasini silmek uzeresiniz.");
        if (popup_confirm(msg)) {
          if (fremove(file_list[file_idx]) == -1) {
            draw_box(15, 8, 50, 6, 0x4C, " HATA ");
            vga_set_cursor(17, 10);
            vga_puts("Dosya silinemedi! (Bilinmeyen Hata)", 0x4F);
            vga_set_cursor(17, 11);
            vga_puts("Kapatmak icin ESC'ye basin...", 0x4F);
            while (input_get().key != 27)
              ;
          }
          scan_root_dir();
        }
        needs_redraw = 1;
      }
    }
    if (k == '\n') { /* ENTER */
      /* ".." girdisi seçildi mi? */
      if (current_dir_id >= 0 && selected_index == 0) {
        fs_change_dir("..");
        scan_root_dir();
        selected_index = 0;
        needs_redraw = 1;
      } else {
        int file_idx = (current_dir_id >= 0) ? selected_index - 1 : selected_index;
        if (file_idx >= 0 && file_idx < file_count) {
          if (file_is_dir[file_idx]) {
            /* Klasöre gir */
            fs_change_dir(file_list[file_idx]);
            scan_root_dir();
            selected_index = 0;
            needs_redraw = 1;
          } else {
            /* Dosya veya uygulama aç */
            char *sel = file_list[file_idx];
            if (strcmp(sel, "SAAT.APP") == 0)
              app_clock();
            else if (strcmp(sel, "EDIT.APP") == 0)
              app_editor(0);
            else if (strcmp(sel, "HESAP.APP") == 0)
              app_calculator();
            else if (strcmp(sel, "TETRIS.APP") == 0)
              game_tetris();
            else if (strcmp(sel, "SNAKE.APP") == 0)
              game_snake();
            else if (strcmp(sel, "PONG.APP") == 0)
              game_pong();
            else if (strcmp(sel, "PACMAN.APP") == 0)
              game_pacman();
            else if (strcmp(sel, "XOX.APP") == 0)
              game_xox();
            else if (strcmp(sel, "MINE.APP") == 0)
              game_minesweeper();
            else if (strcmp(sel, "GEZGIN.APP") == 0)
              app_manager();
            else if (strcmp(sel, "SYSMON.APP") == 0)
              app_system_monitor();
            else
              app_editor(sel);

            scan_root_dir();
            needs_redraw = 1;
          }
        }
      }
    }
  }
}
