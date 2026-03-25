#include "../include/keyboard.h"
#include "../include/vga.h"
#include "../include/pit.h"
#include <stdint.h>

// PONG OYUNU - ZAMAN TABANLI AKICI SURUM
#define P_GENISLIK 74
#define P_YUKSEKLIK 18
#define PAD_BOYUTU 3
#define OFFSET_X 3
#define OFFSET_Y 4

/* Zamanlama sabitleri (milisaniye) */
#define RAKET_MS   35   /* Raket hareket periyodu */
#define TOP_MS     50   /* Top hareket periyodu */
#define AI_MS      45   /* CPU AI guncelleme periyodu */
#define SERVIS_MS  800  /* Skor sonrasi bekleme */

static int top_x, top_y, top_dx, top_dy;
static int p1_y, p2_y, skor1, skor2;
static int eski_top_x, eski_top_y, eski_p1, eski_p2;
static int oyun_modu = 0;
static uint32_t p_seed = 123;
static int p_rand(void) {
  p_seed = p_seed * 1103515245 + 12345;
  return (p_seed >> 16) & 0x7FFF;
}

static void p_sahayi_ciz(void) {
  vga_clear(0x00);

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

  vga_set_cursor(2, 1);
  vga_putc('+', 0x0B);
  for (int i = 0; i < 74; i++) vga_putc('=', 0x0B);
  vga_putc('+', 0x0B);

  vga_set_cursor(2, 2);
  vga_putc('|', 0x0B);
  vga_set_cursor(77, 2);
  vga_putc('|', 0x0B);

  vga_set_cursor(2, 3);
  vga_putc('+', 0x0B);
  for (int i = 0; i < 74; i++) vga_putc('=', 0x0B);
  vga_putc('+', 0x0B);

  vga_set_cursor(38, 2);
  vga_puts("PONG", 0x0E);

  for (int y = 4; y < 22; y++) {
    vga_set_cursor(2, y);
    vga_putc('|', 0x0F);
    vga_set_cursor(77, y);
    vga_putc('|', 0x0F);
  }

  /* Orta Saha Filesi */
  for (int y = 0; y < P_YUKSEKLIK; y++) {
    vga_set_cursor(40, OFFSET_Y + y);
    vga_putc((y % 2) ? '|' : ' ', 0x07);
  }

  vga_set_cursor(2, 22);
  vga_putc('+', 0x0B);
  for (int i = 0; i < 74; i++) vga_putc('=', 0x0B);
  vga_putc('+', 0x0B);

  vga_set_cursor(4, 23);
  if (oyun_modu == 1)
    vga_puts("P1: W/S   P2: OK TUSLARI   F1: YENI   ESC: CIKIS", 0x0F);
  else
    vga_puts("P1: W/S   CPU: Y.ZEKA      F1: YENI   ESC: CIKIS", 0x0F);
}

static void p_skorlari_yaz(void) {
  vga_set_cursor(10, 2);
  vga_puts("P1: ", 0x0B);
  vga_putc(skor1 + '0', 0x0F);

  vga_set_cursor(65, 2);
  if (oyun_modu == 1)
    vga_puts("P2: ", 0x0C);
  else
    vga_puts("CPU: ", 0x0C);
  vga_putc(skor2 + '0', 0x0F);
}

static void p_ciz(void) {
  int render_rx = OFFSET_X + top_x;
  int render_ry = OFFSET_Y + top_y;
  int render_ex = OFFSET_X + eski_top_x;
  int render_ey = OFFSET_Y + eski_top_y;

  if (eski_top_x != top_x || eski_top_y != top_y) {
    vga_set_cursor(render_ex, render_ey);
    if (render_ex == 40 && ((eski_top_y) % 2 != 0))
      vga_putc('|', 0x07);
    else
      vga_putc(' ', 0x00);
  }

  if (eski_p1 != p1_y)
    for (int i = 0; i < PAD_BOYUTU; i++) {
      vga_set_cursor(OFFSET_X, OFFSET_Y + eski_p1 + i);
      vga_putc(' ', 0x00);
    }
  if (eski_p2 != p2_y)
    for (int i = 0; i < PAD_BOYUTU; i++) {
      vga_set_cursor(OFFSET_X + P_GENISLIK - 1, OFFSET_Y + eski_p2 + i);
      vga_putc(' ', 0x00);
    }

  for (int i = 0; i < PAD_BOYUTU; i++) {
    vga_set_cursor(OFFSET_X, OFFSET_Y + p1_y + i);
    vga_putc('|', 0x09);
  }
  for (int i = 0; i < PAD_BOYUTU; i++) {
    vga_set_cursor(OFFSET_X + P_GENISLIK - 1, OFFSET_Y + p2_y + i);
    vga_putc('|', 0x0C);
  }

  vga_set_cursor(render_rx, render_ry);
  vga_putc('O', 0x0E);

  eski_top_x = top_x;
  eski_top_y = top_y;
  eski_p1 = p1_y;
  eski_p2 = p2_y;
}

static void menu_ciz(void) {
  vga_clear(0x00);
  for (int y = 0; y < 25; y++) {
    vga_set_cursor(0, y);
    vga_puts("||", 0x09);
    vga_set_cursor(78, y);
    vga_puts("||", 0x09);
  }
  int mx = 20;
  int my = 8;
  vga_set_cursor(mx + 10, my);
  vga_puts("PONG - MOD SECIMI", 0x0E);
  vga_set_cursor(mx + 2, my + 4);
  vga_puts("1. TEK KISILIK (Bilgisayara Karsi)", 0x07);
  vga_set_cursor(mx + 2, my + 6);
  vga_puts("2. IKI KISILIK (Arkadasinla Oyna)", 0x07);
  vga_set_cursor(mx + 5, my + 10);
  vga_puts("Seciminiz: [1] veya [2]", 0x0F);
  vga_swap_buffers();
}

void game_pong(void) {
mod_secimi:
  menu_ciz();
  while (1) {
    key_event_t ev = input_get();
    if (!ev.pressed) continue;
    int k = ev.key;
    if (k == '1') { oyun_modu = 0; break; }
    if (k == '2') { oyun_modu = 1; break; }
    if (k == 27)  { vga_clear(0x07); return; }
  }

yeni_pong_oyunu:
  top_x = P_GENISLIK / 2;
  top_y = P_YUKSEKLIK / 2;
  top_dx = (p_rand() % 2 == 0) ? 1 : -1;
  top_dy = (p_rand() % 2 == 0) ? 1 : -1;

  p1_y = P_YUKSEKLIK / 2 - PAD_BOYUTU / 2;
  p2_y = p1_y;
  skor1 = 0;
  skor2 = 0;

  eski_top_x = top_x;
  eski_top_y = top_y;
  eski_p1 = p1_y;
  eski_p2 = p2_y;

  p_sahayi_ciz();
  p_skorlari_yaz();
  vga_swap_buffers();

  while (input_available()) input_get();

  uint64_t last_raket  = pit_get_millis();
  uint64_t last_top    = pit_get_millis();
  uint64_t last_ai     = pit_get_millis();
  int      servis_bekleme = 0;
  uint64_t servis_zaman = 0;

  while (skor1 < 5 && skor2 < 5) {
    uint64_t now = pit_get_millis();

    /* Input oku */
    keyboard_poll();
    while (input_available()) {
      key_event_t ev = input_get();
      if (!ev.pressed) continue;
      if (ev.key == 27) { vga_clear(0x07); goto mod_secimi; }
      if (ev.key == KEY_F1) goto yeni_pong_oyunu;
    }

    /* Servis bekleme kontrolu */
    if (servis_bekleme) {
      if (now - servis_zaman >= SERVIS_MS) {
        servis_bekleme = 0;
        last_top = now;
      } else {
        p_ciz();
        vga_swap_buffers();
        for (volatile int i = 0; i < 100; i++) __asm__ volatile("pause");
        continue;
      }
    }

    /* Raket hareketi - zaman tabanli */
    if (now - last_raket >= RAKET_MS) {
      last_raket = now;

      /* P1 raket */
      if ((key_state['w'] || key_state['W'] || (oyun_modu == 0 && key_state[KEY_UP])) && p1_y > 0)
        p1_y--;
      if ((key_state['s'] || key_state['S'] || (oyun_modu == 0 && key_state[KEY_DOWN])) && p1_y < P_YUKSEKLIK - PAD_BOYUTU)
        p1_y++;

      /* P2 raket (2 kisilik) */
      if (oyun_modu == 1) {
        if (key_state[KEY_UP] && p2_y > 0) p2_y--;
        if (key_state[KEY_DOWN] && p2_y < P_YUKSEKLIK - PAD_BOYUTU) p2_y++;
      }
    }

    /* CPU AI - zaman tabanli */
    if (oyun_modu == 0 && (now - last_ai >= AI_MS)) {
      last_ai = now;
      int raket_merkezi = p2_y + PAD_BOYUTU / 2;
      /* %90 dogru takip, %10 hata */
      if (p_rand() % 10 != 0) {
        if (top_dx > 0) {
          if (top_y > raket_merkezi && p2_y < P_YUKSEKLIK - PAD_BOYUTU) p2_y++;
          if (top_y < raket_merkezi && p2_y > 0) p2_y--;
        }
      }
    }

    /* Top hareketi - zaman tabanli */
    if (now - last_top >= TOP_MS) {
      last_top = now;
      top_x += top_dx;
      top_y += top_dy;

      if (top_y <= 0 || top_y >= P_YUKSEKLIK - 1)
        top_dy = -top_dy;

      /* Sol Raket */
      if (top_x == 1 && top_y >= p1_y && top_y < p1_y + PAD_BOYUTU)
        top_dx = 1;

      /* Sag Raket */
      if (top_x == P_GENISLIK - 2 && top_y >= p2_y && top_y < p2_y + PAD_BOYUTU)
        top_dx = -1;

      /* Gol! */
      if (top_x <= 0) {
        skor2++;
        top_x = P_GENISLIK / 2;
        top_y = P_YUKSEKLIK / 2;
        top_dx = 1;
        servis_bekleme = 1;
        servis_zaman = now;
        p_sahayi_ciz();
        p_skorlari_yaz();
        vga_swap_buffers();
      }
      if (top_x >= P_GENISLIK - 1) {
        skor1++;
        top_x = P_GENISLIK / 2;
        top_y = P_YUKSEKLIK / 2;
        top_dx = -1;
        servis_bekleme = 1;
        servis_zaman = now;
        p_sahayi_ciz();
        p_skorlari_yaz();
        vga_swap_buffers();
      }
    }

    p_ciz();
    vga_swap_buffers();

    /* Kucuk bekleme - gereksiz CPU kullanimi onlemek icin */
    for (volatile int d = 0; d < 100; d++)
      __asm__ volatile("pause");
  }

  vga_set_cursor(30, 12);
  if (skor1 > skor2)
    vga_puts(" P1 KAZANDI! ", 0x2E);
  else
    vga_puts(" P2 KAZANDI! ", 0x4E);
  vga_swap_buffers();

  while (1) {
    key_event_t ev = input_get();
    if (!ev.pressed) continue;
    int k = ev.key;
    if (k == 27) goto mod_secimi;
    if (k == KEY_F1) goto yeni_pong_oyunu;
  }
}