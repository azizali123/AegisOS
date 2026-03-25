#include "../include/keyboard.h"
#include "../include/vga.h"
#include <stdint.h>

// TETRIS - GELISTIRILMIS ARAYUZ (Pacman Stili)
// Bloklar: [] (2 karakter genisliginde)
// Duzen: Ortalanmis

// OYUN ALANI AYARLARI
#define BW 10     // Tahta Genisligi (Kare sayisi)
#define BH 28     // Tahta Yuksekligi (Artirilmis yukseklik)
#define W_SCALE 2 // Her kare 2 karakter

// CERCEVE HESAPLAMALARI
// Ekran: 1024x768 (Font 8x16) -> ~128x48 Karakter Kapasitesi
// Oyun Alani: 28 satir
#define WW 38
#define WX 28
#define WY 2 // Yukaridan baslangic

// GLOBAL DEGISKENLER
static int board[BH][BW];
static int piece[4][4], px, py;
static int cur_type, next_type;
static int t_score, t_level, t_lines, t_over;

// TETRIS SEKILLERI (7 Adet)
static const int shapes[7][4][4] = {
    {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // I
    {{1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // J
    {{0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // L
    {{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // O
    {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // S
    {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // T
    {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}  // Z
};

// RENKLER
static const uint8_t tc[7] = {0x0B, 0x09, 0x06, 0x0E, 0x0A, 0x0D, 0x0C};

// RASTGELE
static uint32_t ts = 12345;
static int tr(void) {
  ts = ts * 1103515245 + 12345;
  return (ts >> 16) & 0x7FFF;
}

static void t_copy(int dst[4][4], const int src[4][4]) {
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      dst[i][j] = src[i][j];
}

static void t_load(int type) {
  cur_type = type % 7;
  t_copy(piece, shapes[cur_type]);
  px = BW / 2 - 2;
  py = -1;
}

static int t_hit(int nx, int ny, int p[4][4]) {
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (p[i][j]) {
        int bx = nx + j, by = ny + i;
        if (bx < 0 || bx >= BW || by >= BH)
          return 1;
        if (by >= 0 && board[by][bx])
          return 1;
      }
  return 0;
}

static void t_rotate(void) {
  int tmp[4][4];
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      tmp[i][j] = piece[3 - j][i];
  if (!t_hit(px, py, tmp))
    t_copy(piece, tmp);
}

static void t_merge(void) {
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (piece[i][j]) {
        int by = py + i, bx = px + j;
        if (by >= 0 && by < BH && bx >= 0 && bx < BW)
          board[by][bx] = cur_type + 1;
      }
}

static void t_clear(void) {
  int cl = 0;
  for (int y = BH - 1; y >= 0; y--) {
    int f = 1;
    for (int x = 0; x < BW; x++)
      if (!board[y][x])
        f = 0;
    if (f) {
      cl++;
      for (int yy = y; yy > 0; yy--)
        for (int x = 0; x < BW; x++)
          board[yy][x] = board[yy - 1][x];
      for (int x = 0; x < BW; x++)
        board[0][x] = 0;
      y++;
    }
  }
  if (cl) {
    t_lines += cl;
    t_score += cl * cl * 100;
    t_level = t_lines / 10;
  }
}

// --- YENI CERCEVE SISTEMI (PACMAN STILI) ---
static void t_draw_border(void) {
  vga_clear(0x00);

  // --- DIS CERCEVE (Yildizlar) ---
  // Ekran yetenegimiz yuksek (48 satir), cerceveyi genisletebiliriz.
  // 35 satir yeterli (4 header + 28 oyun + 3 footer)
  int cerceve_h = 36;

  for (int x = 0; x < 80; x++) {
    vga_set_cursor(x, 0);
    vga_putc('*', 0x09);
    vga_set_cursor(x, cerceve_h);
    vga_putc('*', 0x09);
  }
  for (int y = 0; y <= cerceve_h; y++) {
    vga_set_cursor(0, y);
    vga_puts("||", 0x09);
    vga_set_cursor(78, y);
    vga_puts("||", 0x09);
  }

  // --- UST KUTU ---
  vga_set_cursor(2, 1);
  vga_putc('+', 0x0B);
  for (int i = 0; i < 74; i++)
    vga_putc('=', 0x0B);
  vga_putc('+', 0x0B);

  vga_set_cursor(2, 2);
  vga_putc('|', 0x0B);
  vga_set_cursor(77, 2);
  vga_putc('|', 0x0B);

  vga_set_cursor(2, 3);
  vga_putc('+', 0x0B);
  for (int i = 0; i < 74; i++)
    vga_putc('=', 0x0B);
  vga_putc('+', 0x0B);

  // Baslik
  vga_set_cursor(36, 2);
  vga_puts("TETRIS", 0x0E);

  // --- OYUN ALANI DUVARLARI ---
  // Oyun alani Baslangic Y = 4
  // Bitis Y = 4 + 28 = 32
  // Duvarlar 4'ten 32'ye kadar iner
  for (int y = 4; y < 4 + BH; y++) {
    vga_set_cursor(2, y);
    vga_putc('|', 0x0F);
    vga_set_cursor(77, y);
    vga_putc('|', 0x0F);
  }

  // --- ALT KUTU ---
  int alt_y = 4 + BH; // 32
  vga_set_cursor(2, alt_y);
  vga_putc('+', 0x0B);
  for (int i = 0; i < 74; i++)
    vga_putc('=', 0x0B);
  vga_putc('+', 0x0B);

  // Alt Bilgi
  vga_set_cursor(4, alt_y + 1);
  vga_puts("OKLAR: YON/CEVIR   F1: YENI OYUN   ESC: CIKIS", 0x0F);
}

// ARABIRIMI CIZME (Statik kisimlar - Oyun ici)
static void t_draw_frame_static(void) {
  // Tetris Tahta Cercevesi (Iceride)
  int tx = WX - 1;
  int ty = WY;
  int tw = BW * W_SCALE;

  // Tahta yani cizgileri
  for (int i = 0; i < BH; i++) {
    vga_set_cursor(tx, ty + i);
    vga_putc('|', 0x08);
    vga_set_cursor(tx + tw + 1, ty + i);
    vga_putc('|', 0x08);
  }

  // SAG BILGI KUTUSU
  int ix = WX + (BW * W_SCALE) + 4;
  vga_set_cursor(ix, WY + 2);
  vga_puts("SKOR:", 0x07);
  vga_set_cursor(ix, WY + 5);
  vga_puts("SEVIYE:", 0x07);
  vga_set_cursor(ix, WY + 8);
  vga_puts("SONRAKI:", 0x07);
}

// EKRAN GUNCELLEME
static void t_draw(void) {
  // Skor
  int ix = WX + (BW * W_SCALE) + 4;
  char b[10];
  int s = t_score, k = 0;
  if (s == 0)
    b[k++] = '0';
  else
    while (s > 0) {
      b[k++] = (s % 10) + '0';
      s /= 10;
    }
  b[k] = 0;
  for (int i = 0; i < k / 2; i++) {
    char t = b[i];
    b[i] = b[k - 1 - i];
    b[k - 1 - i] = t;
  }
  vga_set_cursor(ix, WY + 3);
  vga_puts(b, 0x0E);

  // Seviye
  s = t_level;
  k = 0;
  if (s == 0)
    b[k++] = '0';
  else
    while (s > 0) {
      b[k++] = (s % 10) + '0';
      s /= 10;
    }
  b[k] = 0;
  for (int i = 0; i < k / 2; i++) {
    char t = b[i];
    b[i] = b[k - 1 - i];
    b[k - 1 - i] = t;
  }
  vga_set_cursor(ix, WY + 6);
  vga_puts(b, 0x0E);

  // Tahta Render
  int current_display[BH][BW];
  for (int y = 0; y < BH; y++)
    for (int x = 0; x < BW; x++)
      current_display[y][x] = board[y][x];

  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (piece[i][j]) {
        int by = py + i, bx = px + j;
        if (by >= 0 && by < BH && bx >= 0 && bx < BW)
          current_display[by][bx] = cur_type + 1;
      }

  for (int y = 0; y < BH; y++) {
    for (int x = 0; x < BW; x++) {
      vga_set_cursor(WX + x * W_SCALE, WY + y);
      int val = current_display[y][x];
      if (val) {
        uint8_t c = tc[(val - 1) % 7];
        vga_putc('[', c);
        vga_putc(']', c);
      } else {
        vga_puts(" .", 0x08);
      }
    }
  }

  // Sonraki Parca
  int nx = ix;
  int ny = WY + 10;
  for (int i = 0; i < 4; i++) {
    vga_set_cursor(nx, ny + i);
    vga_puts("        ", 0);
  }
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) {
      if (shapes[next_type % 7][i][j]) {
        vga_set_cursor(nx + j * W_SCALE, ny + i);
        vga_putc('[', tc[next_type % 7]);
        vga_putc(']', tc[next_type % 7]);
      }
    }
  vga_swap_buffers();
}

void game_tetris(void) {
  // Baslangic sifirlamalari
  for (int y = 0; y < BH; y++)
    for (int x = 0; x < BW; x++)
      board[y][x] = 0;
  t_score = 0;
  t_level = 0;
  t_over = 0;

  next_type = tr() % 7;
  t_load(tr() % 7);
  next_type = tr() % 7;
  int tick = 0;

  t_draw_border();
  t_draw_frame_static();
  vga_swap_buffers();

  while (input_available()) input_get();

  while (!t_over) {
    int speed_factor = (35 - t_level);
    if (speed_factor < 5)
      speed_factor = 5;

    for (int p = 0; p < 10; p++) {
      int drew = 0;
      while (input_available()) {
        key_event_t ev = input_get();
        if (!ev.pressed) continue;
        int k = ev.key;

        if (k == 27) {
          vga_clear(0x07);
          return;
        }
        if (k == KEY_F1) {
          for (int y = 0; y < BH; y++)
            for (int x = 0; x < BW; x++)
              board[y][x] = 0;
          t_score = 0;
          t_level = 0;
          t_over = 0;
          t_load(tr() % 7);
          next_type = tr() % 7;
          t_draw_border();
          t_draw_frame_static();
          vga_swap_buffers();
        }
        if ((k == KEY_LEFT || k == 'A' || k == 'a') && !t_hit(px - 1, py, piece))
          px--;
        if ((k == KEY_RIGHT || k == 'D' || k == 'd') && !t_hit(px + 1, py, piece))
          px++;
        if ((k == KEY_UP || k == 'W' || k == 'w'))
          t_rotate();
        if ((k == KEY_DOWN || k == 'S' || k == 's') && !t_hit(px, py + 1, piece)) {
          py++;
          t_score++;
        }
        drew = 1;
      }
      if (drew) t_draw();
    }

    if (tick++ > speed_factor) {
      tick = 0;
      if (!t_hit(px, py + 1, piece))
        py++;
      else {
        t_merge();
        t_clear();
        t_load(next_type);
        next_type = tr() % 7;
        if (t_hit(px, py, piece))
          t_over = 1;
      }
      t_draw();
    }
    for (volatile int d = 0; d < 3875000; d++) {
      if ((d & 0x7FF) == 0)
        keyboard_poll();
    }
  }

  vga_set_cursor(36, 12);
  vga_puts(" OYUN BITTI ", 0x4F);
  vga_swap_buffers();
  while (1) {
      key_event_t ev = input_get();
      if (ev.pressed && ev.key == 27) break;
  }
  vga_clear(0x07);
}
