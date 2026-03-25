#include "../include/filesystem.h"
#include "../include/input.h"
#include "../include/vga.h"
#include "../include/tui.h"
#include <stdint.h>

/* AZGIR EDITOR - Dinamik pencere boyutu */
#define MAX_BUFFER 4096

/* Dinamik pencere hesaplama */
static int WX, WY, WW, WH;

static void calc_window_size(void) {
    int screen_cols = (int)(vga_get_width() / 8);
    int screen_rows = (int)(vga_get_height() / 16);

    /* Pencereyi ekran boyutuna gore hesapla */
    int margin_x = 3;
    int margin_y = 1;

    WW = screen_cols - 2 * margin_x;
    WH = screen_rows - 2 * margin_y;

    /* Minimum boyut guvencesi */
    if (WW < 40) WW = 40;
    if (WH < 15) WH = 15;

    /* Maksimum boyut guvencesi (ekrandan tasmasin) */
    if (WW > screen_cols - 2) WW = screen_cols - 2;
    if (WH > screen_rows - 1) WH = screen_rows - 1;

    WX = (screen_cols - WW) / 2;
    WY = margin_y;

    /* Son guvence: kesinlikle ekran icinde */
    if (WX + WW > screen_cols) WW = screen_cols - WX;
    if (WY + WH > screen_rows) WH = screen_rows - WY;
}

static char text_buffer[MAX_BUFFER];
static int cursor_idx = 0;
static int max_idx = 0;
static char current_filename[32];

static void draw_border(void) {
  aegis_theme_t* t = theme_get_current();
  /* Ust */
  vga_set_cursor(WX, WY);
  vga_putc('|', t->border_color);
  for (int i = 0; i < WW - 2; i++)
    vga_putc('*', t->border_color);
  vga_putc('|', t->border_color);

  /* Yanlar */
  for (int i = 1; i < WH - 1; i++) {
    vga_set_cursor(WX, WY + i);
    vga_putc('|', t->border_color);
    vga_set_cursor(WX + WW - 1, WY + i);
    vga_putc('|', t->border_color);
  }

  /* Alt */
  vga_set_cursor(WX, WY + WH - 1);
  vga_putc('|', t->border_color);
  for (int i = 0; i < WW - 2; i++)
    vga_putc('*', t->border_color);
  vga_putc('|', t->border_color);
}

static void draw_interface(void) {
  aegis_theme_t* t = theme_get_current();
  int rx = WX + 2;
  int ry = WY + 1;
  int inner_w = WW - 4;

  /* Ust ayirici */
  vga_set_cursor(rx, ry);
  vga_putc('+', t->border_color);
  for (int i = 0; i < inner_w - 2; i++) vga_putc('=', t->border_color);
  vga_putc('+', t->border_color);

  /* Satir ve karakter hesapla */
  int line = 1, col = 1;
  for (int i = 0; i < cursor_idx; i++) {
    if (text_buffer[i] == '\n') { line++; col = 1; }
    else col++;
  }

  /* Durum satiri */
  int status_y = ry + 1;
  vga_set_cursor(rx, status_y);

  /* Satiri temizle */
  for (int i = 0; i < inner_w; i++) vga_putc(' ', t->bg_color);

  vga_set_cursor(rx, status_y);
  vga_puts("| AEGIS OS EDITOR", t->fg_color);
  if (current_filename[0] != 0) {
    vga_puts("[", t->accent_color);
    vga_puts(current_filename, t->accent_color);
    vga_puts("]", t->accent_color);
  } else {
    vga_puts("[Adsiz]", t->fg_color);
  }

  /* Satir/Karakter bilgisi (sag tarafa yerlestir) */
  int info_x = rx + inner_w - 30;
  if (info_x < rx + 20) info_x = rx + 20;
  vga_set_cursor(info_x, status_y);
  vga_puts("SATIR: ", t->fg_color);

  char buf[16];
  int temp = line, k = 0;
  if (temp == 0) buf[k++] = '0';
  else while (temp > 0) { buf[k++] = (temp % 10) + '0'; temp /= 10; }
  for (int i = 0; i < k / 2; i++) { char t_char = buf[i]; buf[i] = buf[k - 1 - i]; buf[k - 1 - i] = t_char; }
  buf[k] = 0;
  vga_puts(buf, t->accent_color);

  vga_puts(" KAR:", t->fg_color);
  temp = col; k = 0;
  if (temp == 0) buf[k++] = '0';
  else while (temp > 0) { buf[k++] = (temp % 10) + '0'; temp /= 10; }
  for (int i = 0; i < k / 2; i++) { char t_char = buf[i]; buf[i] = buf[k - 1 - i]; buf[k - 1 - i] = t_char; }
  buf[k] = 0;
  vga_puts(buf, t->accent_color);

  /* Status sag siniri */
  vga_set_cursor(rx + inner_w - 1, status_y);
  vga_puts("|", t->border_color);

  /* Durum altindaki ayirici */
  vga_set_cursor(rx, ry + 2);
  vga_putc('+', t->border_color);
  for (int i = 0; i < inner_w - 2; i++) vga_putc('=', t->border_color);
  vga_putc('+', t->border_color);

  /* Metin alani */
  int ty = ry + 3;
  int tx = rx + 2;
  int max_rows = WH - 8;
  int max_cols = inner_w - 4;

  if (max_rows < 3) max_rows = 3;
  if (max_cols < 10) max_cols = 10;

  /* Temizle */
  for (int i = 0; i < max_rows; i++) {
    vga_set_cursor(tx, ty + i);
    for (int j = 0; j < max_cols; j++) vga_putc(' ', t->bg_color);
  }

  int crow = 0;
  int ccol = 0;

  vga_set_cursor(tx, ty);
  for (int i = 0; i < max_idx; i++) {
    char c = text_buffer[i];
    if (c == '\n') {
      crow++;
      ccol = 0;
      if (crow >= max_rows) break;
      vga_set_cursor(tx, ty + crow);
    } else {
      if (ccol < max_cols) {
        vga_putc(c, t->fg_color);
        ccol++;
      }
    }
  }

  /* Imleç */
  if (crow < max_rows && ccol < max_cols) {
    vga_set_cursor(tx + ccol, ty + crow);
    vga_putc('_', t->accent_color);
  }

  /* Footer */
  int fy = ry + WH - 5;
  if (fy <= ty + max_rows) fy = ty + max_rows + 1;
  if (fy + 2 >= WY + WH - 1) fy = WY + WH - 4;

  vga_set_cursor(rx, fy);
  vga_putc('+', t->border_color);
  for (int i = 0; i < inner_w - 2; i++) vga_putc('=', t->border_color);
  vga_putc('+', t->border_color);

  vga_set_cursor(rx, fy + 1);
  /* Footer metnini pencere genisligine gore kes */
  const char *footer = "| ESC:CIKIS  OKLAR:YON  ENTER:YENI SATIR  BKSP:SIL  F5:KAYDET";
  vga_puts(footer, t->fg_color);
  /* Kalan yeri doldur */
  int cur_x, cur_y;
  vga_get_cursor(&cur_x, &cur_y);
  while (cur_x < rx + inner_w - 1) {
    vga_putc(' ', t->bg_color);
    cur_x++;
  }
  vga_putc('|', t->border_color);

  vga_set_cursor(rx, fy + 2);
  vga_putc('+', t->border_color);
  for (int i = 0; i < inner_w - 2; i++) vga_putc('=', t->border_color);
  vga_putc('+', t->border_color);
}

static void save_file(void) {
  if (current_filename[0] == 0) {
    const char *default_name = "YENI.TXT";
    int i = 0;
    for (; default_name[i]; i++)
      current_filename[i] = default_name[i];
    current_filename[i] = 0;
  }

  File *f = fopen(current_filename, "w");
  if (f) {
    fwrite(text_buffer, 1, max_idx, f);
    fclose(f);
    vga_set_cursor(WX + 2, WY + WH - 2);
    vga_puts("DOSYA KAYDEDILDI!   ", 0x0A);
  } else {
    vga_set_cursor(WX + 2, WY + WH - 2);
    vga_puts("HATA: KAYDEDILEMEDI!", 0x0C);
  }
}

void app_editor(const char *filename) {
  /* Pencere boyutunu hesapla */
  calc_window_size();

  for (int i = 0; i < MAX_BUFFER; i++)
    text_buffer[i] = 0;
  cursor_idx = 0;
  max_idx = 0;
  current_filename[0] = 0;

  if (filename) {
    int i = 0;
    for (; filename[i] && i < 31; i++)
      current_filename[i] = filename[i];
    current_filename[i] = 0;

    File *f = fopen(filename, "r");
    if (f) {
      int bytes = fread(text_buffer, 1, MAX_BUFFER - 1, f);
      text_buffer[bytes] = 0;
      max_idx = bytes;
      fclose(f);
    }
  }

  vga_clear(0x00);
  draw_border();
  draw_interface();
  vga_swap_buffers();

  while (1) {
    key_event_t ev = input_get();
    if (!ev.pressed) continue;
    int k = ev.key;

    if (k == 27) {
      vga_clear(0x00);
      vga_swap_buffers();
      return;
    }
    if (k == KEY_F5) {
      save_file();
      vga_swap_buffers();
      continue;
    }

    if (k == '\b') {
      if (cursor_idx > 0) {
        cursor_idx--;
        for (int i = cursor_idx; i < max_idx; i++)
          text_buffer[i] = text_buffer[i + 1];
        max_idx--;
        draw_interface();
        vga_swap_buffers();
      }
    } else if (k == '\n') {
      if (max_idx < MAX_BUFFER - 1) {
        for (int i = max_idx; i > cursor_idx; i--)
          text_buffer[i] = text_buffer[i - 1];
        text_buffer[cursor_idx] = '\n';
        cursor_idx++;
        max_idx++;
        draw_interface();
        vga_swap_buffers();
      }
    } else if (k >= 32 && k <= 255) {
      if (max_idx < MAX_BUFFER - 1) {
        for (int i = max_idx; i > cursor_idx; i--)
          text_buffer[i] = text_buffer[i - 1];
        text_buffer[cursor_idx] = (char)k;
        cursor_idx++;
        max_idx++;
        draw_interface();
        vga_swap_buffers();
      }
    } else if (k == KEY_LEFT) {
      if (cursor_idx > 0) {
        cursor_idx--;
        draw_interface();
        vga_swap_buffers();
      }
    } else if (k == KEY_RIGHT) {
      if (cursor_idx < max_idx) {
        cursor_idx++;
        draw_interface();
        vga_swap_buffers();
      }
    } else if (k == KEY_UP) {
      if (cursor_idx > 0) {
        int line_start = cursor_idx - 1;
        while (line_start >= 0 && text_buffer[line_start] != '\n') line_start--;
        if (line_start >= 0) {
          int prev = line_start - 1;
          while (prev >= 0 && text_buffer[prev] != '\n') prev--;
          int col2 = cursor_idx - (line_start + 1);
          int plen = line_start - (prev + 1);
          col2 = (col2 > plen) ? plen : col2;
          cursor_idx = prev + 1 + col2;
          draw_interface();
          vga_swap_buffers();
        }
      }
    } else if (k == KEY_DOWN) {
      int line_start = cursor_idx - 1;
      while (line_start >= 0 && text_buffer[line_start] != '\n') line_start--;
      int col2 = cursor_idx - (line_start + 1);
      int next_line = cursor_idx;
      while (next_line < max_idx && text_buffer[next_line] != '\n') next_line++;
      if (next_line < max_idx) {
        int nnext = next_line + 1;
        while (nnext < max_idx && text_buffer[nnext] != '\n') nnext++;
        int nlen = nnext - (next_line + 1);
        col2 = (col2 > nlen) ? nlen : col2;
        cursor_idx = next_line + 1 + col2;
        draw_interface();
        vga_swap_buffers();
      }
    }
  }
}
