#include "../include/keyboard.h"
#include "../include/vga.h"
#include <stdint.h>

// YILAN OYUNU - PACMAN STILI ARAYUZ
// Oyun Alani: 30x18 (Sigmasi icin)

#define SW 30
#define SH 18
#define WX                                                                     \
  9 // (80 - 64) / 2 = 8. Ortalamak icin -> 9 (Kareler 2 genislikte: 30*2=60.
    // 80-60=20. WX=10) Pacman frame kullanacagiz: x=2 den x=77 ye kadar 75
    // birimlik alan var. Bizim yilan 60 genislikte. 75-60=15. Ortala ~7-8
    // ofset. WX = 10 diyelim (Frame x=2'de basliyor, +8 margin)
#define GL_OFFSET_X 10
#define GL_OFFSET_Y 4

static int snake_x[300], snake_y[300];
static int snake_len;
static int dir_x, dir_y;
static int apple_x, apple_y;
static int score = 0, level = 1;
static int game_over;

static uint32_t sseed = 98765;
static int s_rand(void) {
  sseed = sseed * 1103515245 + 12345;
  return (sseed >> 16) & 0x7FFF;
}

// PACMAN-STILI CERCEVE
static void draw_border_snake(void) {
  vga_clear(0x00);

  // --- DIS CERCEVE (Yildizlar) ---
  for (int x = 0; x < 80; x++) {
    vga_set_cursor(x, 0);
    vga_putc('*', 0x09);
    vga_set_cursor(x, 24);
    vga_putc('*', 0x09);
  }
  for (int y = 0; y < 25; y++) {
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
  vga_set_cursor(35, 2);
  vga_puts("YILAN OYUNU", 0x0E);

  // --- OYUN ALANI DUVARLARI ---
  for (int y = 4; y < 22; y++) {
    vga_set_cursor(2, y);
    vga_putc('|', 0x0F);
    vga_set_cursor(77, y);
    vga_putc('|', 0x0F);
  }

  // --- ALT KUTU ---
  vga_set_cursor(2, 22);
  vga_putc('+', 0x0B);
  for (int i = 0; i < 74; i++)
    vga_putc('=', 0x0B);
  vga_putc('+', 0x0B);

  // Alt Bilgi
  vga_set_cursor(4, 23);
  vga_puts(" YON TUSLARI: Hareket    F1: Yeni Oyun    ESC: Cikis", 0x0F);
}

static void s_draw_apple(int x, int y) {
  vga_set_cursor(GL_OFFSET_X + x * 2, GL_OFFSET_Y + y);
  vga_puts("()", 0x0C);
}

static void spawn_apple(void) {
  int v;
  do {
    apple_x = s_rand() % SW;
    apple_y = s_rand() % SH;
    v = 1;
    for (int i = 0; i < snake_len; i++)
      if (snake_x[i] == apple_x && snake_y[i] == apple_y) {
        v = 0;
        break;
      }
  } while (!v);
  s_draw_apple(apple_x, apple_y);
}

static void s_update_score(void) {
  // Skor bilgisini Alt Kutu icine yaz (Sag taraf)
  vga_set_cursor(60, 23);
  vga_puts("SKOR: ", 0x0E);
  vga_putc((score / 100) + '0', 0x0F);
  vga_putc(((score / 10) % 10) + '0', 0x0F);
  vga_putc((score % 10) + '0', 0x0F);
}

void game_snake(void) {
restart_game:
  draw_border_snake();

  // Oyun Alani Izgarasi (Noktalar) - Sinirlar icinde
  for (int y = 0; y < SH; y++) {
    vga_set_cursor(GL_OFFSET_X, GL_OFFSET_Y + y);
    for (int x = 0; x < SW; x++) {
      vga_puts(" .", 0x08);
    }
  }

  snake_len = 3;
  snake_x[0] = SW / 2;
  snake_y[0] = SH / 2;
  snake_x[1] = SW / 2 - 1;
  snake_y[1] = SH / 2;
  snake_x[2] = SW / 2 - 2;
  snake_y[2] = SH / 2;

  dir_x = 1;
  dir_y = 0;
  score = 0;
  level = 1;
  game_over = 0;

  s_update_score();
  spawn_apple();

  // Yilani Ciz
  for (int i = 0; i < snake_len; i++) {
    vga_set_cursor(GL_OFFSET_X + snake_x[i] * 2, GL_OFFSET_Y + snake_y[i]);
    if (i == 0)
      vga_puts("# ", 0x0A);
    else
      vga_puts("o ", 0x02);
  }
  vga_swap_buffers();

  while (input_available()) input_get();

  while (!game_over) {
    for (int step = 0; step < 20; step++) {
      while (input_available()) {
        key_event_t ev = input_get();
        if (!ev.pressed) continue;
        int k = ev.key;
        if (k == 27) {
          vga_clear(0x07);
          return;
        }
        if (k == KEY_F1)
          goto restart_game;

        if ((k == KEY_UP || k == 'W' || k == 'w') && dir_y != 1) {
          dir_x = 0;
          dir_y = -1;
        }
        if ((k == KEY_DOWN || k == 'S' || k == 's') && dir_y != -1) {
          dir_x = 0;
          dir_y = 1;
        }
        if ((k == KEY_LEFT || k == 'A' || k == 'a') && dir_x != 1) {
          dir_x = -1;
          dir_y = 0;
        }
        if ((k == KEY_RIGHT || k == 'D' || k == 'd') && dir_x != -1) {
          dir_x = 1;
          dir_y = 0;
        }
      }

      for (volatile int d = 0; d < 4000000; d++) {
        if ((d & 0x7FF) == 0)
          keyboard_poll();
      }
    }

    int nx = snake_x[0] + dir_x;
    int ny = snake_y[0] + dir_y;

    if (nx < 0 || nx >= SW || ny < 0 || ny >= SH) {
      game_over = 1;
      break;
    }
    for (int i = 0; i < snake_len; i++)
      if (snake_x[i] == nx && snake_y[i] == ny) {
        game_over = 1;
        break;
      }

    int ate = (nx == apple_x && ny == apple_y);
    int tx = snake_x[snake_len - 1];
    int ty = snake_y[snake_len - 1];

    if (!ate) {
      vga_set_cursor(GL_OFFSET_X + tx * 2, GL_OFFSET_Y + ty);
      vga_puts(" .", 0x08);
    }

    for (int i = snake_len - 1; i > 0; i--) {
      snake_x[i] = snake_x[i - 1];
      snake_y[i] = snake_y[i - 1];
    }
    snake_x[0] = nx;
    snake_y[0] = ny;

    if (ate) {
      if (snake_len < 300)
        snake_len++;
      score += 10;
      if (score % 50 == 0)
        level++;
      s_update_score();
      spawn_apple();
    }

    // Yilani Ciz
    vga_set_cursor(GL_OFFSET_X + nx * 2, GL_OFFSET_Y + ny);
    vga_puts("# ", 0x0A); // Bas

    // Eski basi Govde Yap
    if (snake_len > 1) {
      vga_set_cursor(GL_OFFSET_X + snake_x[1] * 2, GL_OFFSET_Y + snake_y[1]);
      vga_puts("o ", 0x02);
    }
    vga_swap_buffers();
  }

  vga_set_cursor(35, 12);
  vga_puts(" OYUN BITTI ", 0x4F);
  vga_swap_buffers();

  while (1) {
    key_event_t ev = input_get();
    if (!ev.pressed) continue;
    int k = ev.key;
    if (k == 27)
      break;
    if (k == KEY_F1)
      goto restart_game;
  }
  vga_clear(0x07);
}
