#include "../include/keyboard.h"
#include "../include/vga.h"
#include <stdint.h>

// XOX (TIC-TAC-TOE) - PACMAN STILI ARAYUZ
// Ortalanacak.

#define P_GENISLIK 30 // Tahta Genisligi (Karakter)
#define P_YUKSEKLIK 14
#define P_X 25 // 80 - 30 = 50 / 2 = 25
#define P_Y 6  // 4 + 2

static char tahta[3][3];
static int imlec_x = 0, imlec_y = 0;
static int sira = 0;
static int skor_x = 0, skor_o = 0, beraberlik = 0;
static int oyun_bitti = 0;
static int kazanan = 0;
static int oyun_modu = 0;

static uint32_t x_seed = 9999;
static int x_rand(void) {
  x_seed = x_seed * 1103515245 + 12345;
  return (x_seed >> 16) & 0x7FFF;
}

static int kontrol_et(void) {
  // Satir
  for (int i = 0; i < 3; i++) {
    if (tahta[i][0] != ' ' && tahta[i][0] == tahta[i][1] &&
        tahta[i][1] == tahta[i][2])
      return (tahta[i][0] == 'X') ? 1 : 2;
  }
  // Sutun
  for (int i = 0; i < 3; i++) {
    if (tahta[0][i] != ' ' && tahta[0][i] == tahta[1][i] &&
        tahta[1][i] == tahta[2][i])
      return (tahta[0][i] == 'X') ? 1 : 2;
  }
  // Capraz
  if (tahta[0][0] != ' ' && tahta[0][0] == tahta[1][1] &&
      tahta[1][1] == tahta[2][2])
    return (tahta[0][0] == 'X') ? 1 : 2;
  if (tahta[0][2] != ' ' && tahta[0][2] == tahta[1][1] &&
      tahta[1][1] == tahta[2][0])
    return (tahta[0][2] == 'X') ? 1 : 2;

  int dolu = 1;
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      if (tahta[i][j] == ' ')
        dolu = 0;
  if (dolu)
    return 3;

  return 0;
}

static void bilgisayar_oyna(void) {
  int hamle_bulundu = 0;
  int deneme = 0;
  int bos_var = 0;
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      if (tahta[i][j] == ' ')
        bos_var = 1;
  if (!bos_var)
    return;

  while (!hamle_bulundu && deneme < 100) {
    int rx = x_rand() % 3;
    int ry = x_rand() % 3;
    if (tahta[ry][rx] == ' ') {
      tahta[ry][rx] = 'O';
      hamle_bulundu = 1;
    }
    deneme++;
  }
  if (!hamle_bulundu) {
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        if (tahta[i][j] == ' ') {
          tahta[i][j] = 'O';
          hamle_bulundu = 1;
          goto cikis;
        }
  }
cikis:;
}

static void cerceve_ciz_pacman_style(void) {
  vga_clear(0x00);

  // DIS CERCEVE (Yildizlar)
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

  // UST KUTU
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
  vga_set_cursor(38, 2);
  vga_puts("XOX", 0x0E);

  // ALT KUTU
  vga_set_cursor(2, 22);
  vga_putc('+', 0x0B);
  for (int i = 0; i < 74; i++)
    vga_putc('=', 0x0B);
  vga_putc('+', 0x0B);

  // Alt Bilgi
  vga_set_cursor(4, 23);
  vga_puts("OKLAR: Sec   ENTER: Isaretle   F1: Yeni Oyun   ESC: Cikis", 0x0F);
}

static void ekrani_ciz(void) {
  int bx = P_X;
  int by = P_Y;

  // Skorlar (Ust kutunun altinda)
  vga_set_cursor(bx, 4);
  vga_puts("Oyuncu X: ", 0x0C);
  vga_putc(skor_x + '0', 0x0F);
  vga_set_cursor(bx + 18, 4);
  vga_puts("Oyuncu O: ", 0x0B);
  vga_putc(skor_o + '0', 0x0F);

  // Oyun Alani Izgarasi
  int ox = bx + 5;
  int oy = by + 2; // Offset

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      int hx = ox + j * 6;
      int hy = oy + i * 3;

      uint8_t renk = 0x07;
      uint8_t bg = 0x00;

      if (i == imlec_y && j == imlec_x) {
        bg = 0x70;
        renk = 0x70;
      }

      char sembol = tahta[i][j];
      uint8_t sembol_renk = 0x0F;
      if (sembol == 'X')
        sembol_renk = (bg | 0x0C);
      if (sembol == 'O')
        sembol_renk = (bg | 0x0B);
      if (sembol == ' ')
        sembol_renk = renk;

      vga_set_cursor(hx, hy);
      vga_putc('[', renk);
      vga_puts("   ", bg);
      vga_putc(']', renk);
      vga_set_cursor(hx, hy + 1);
      vga_putc('[', renk);
      vga_putc(' ', bg);
      vga_putc(sembol, sembol_renk);
      vga_putc(' ', bg);
      vga_putc(']', renk);
      vga_set_cursor(hx, hy + 2);
      vga_putc('[', renk);
      vga_puts("   ", bg);
      vga_putc(']', renk);
    }
  }

  vga_set_cursor(bx, oy + 10);
  if (!oyun_bitti) {
    if (sira == 0)
      vga_puts("SIRA: X (Kirmizi)", 0x0C);
    else
      vga_puts("SIRA: O (Mavi)   ", 0x0B);
  } else {
    if (kazanan == 1)
      vga_puts("KAZANAN: X!      ", 0xCE);
    else if (kazanan == 2)
      vga_puts("KAZANAN: O!      ", 0xBE);
    else
      vga_puts("BERABERE!        ", 0x0E);
  }
}

static void menu_ciz(void) {
  cerceve_ciz_pacman_style();
  int mx = 20;
  int my = 8;
  vga_set_cursor(mx + 5, my);
  vga_puts("XOX - MOD SECIMI", 0x0E);
  vga_set_cursor(mx + 2, my + 4);
  vga_puts("1. BILGISAYARA KARSI", 0x0F);
  vga_set_cursor(mx + 2, my + 6);
  vga_puts("2. IKI KISILIK", 0x0F);
  vga_set_cursor(mx + 2, my + 10);
  vga_puts("SECINIZ: [1] veya [2]", 0x07);
}

void game_xox(void) {
mod_secimi:
  menu_ciz();
  while (1) {
    int k = keyboard_read_key();
    if (k == '1') {
      oyun_modu = 0;
      break;
    }
    if (k == '2') {
      oyun_modu = 1;
      break;
    }
    if (k == 27) {
      vga_clear(0x07);
      return;
    }
  }
  skor_x = 0;
  skor_o = 0;
  beraberlik = 0;

yeni_oyun:
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      tahta[i][j] = ' ';
  imlec_x = 1;
  imlec_y = 1;
  sira = 0;
  oyun_bitti = 0;
  kazanan = 0;

  cerceve_ciz_pacman_style();
  ekrani_ciz();

  while (keyboard_check_key())
    ;

  while (1) {
    // Bilgisayar hamlesi
    if (oyun_modu == 0 && sira == 1 && !oyun_bitti) {
      for (volatile int d = 0; d < 5000000; d++)
        ;
      bilgisayar_oyna();
      int sonuc = kontrol_et();
      if (sonuc != 0) {
        oyun_bitti = 1;
        kazanan = sonuc;
        if (sonuc == 1)
          skor_x++;
        else if (sonuc == 2)
          skor_o++;
        else
          beraberlik++;
      } else {
        sira = 0;
      }
      ekrani_ciz();
      continue;
    }

    int k = keyboard_read_key();
    if (k == 27) {
      vga_clear(0x07);
      goto mod_secimi;
    }
    if (k == KEY_F1)
      goto yeni_oyun;

    if (!oyun_bitti) {
      // Imlec Hareketi
      if (k == KEY_LEFT && imlec_x > 0)
        imlec_x--;
      if (k == KEY_RIGHT && imlec_x < 2)
        imlec_x++;
      if (k == KEY_UP && imlec_y > 0)
        imlec_y--;
      if (k == KEY_DOWN && imlec_y < 2)
        imlec_y++;

      if ((k == '\n' || k == ' ') && tahta[imlec_y][imlec_x] == ' ') {
        int yapildi = 0;
        if (sira == 0) {
          tahta[imlec_y][imlec_x] = 'X';
          yapildi = 1;
        } else if (oyun_modu == 1) {
          tahta[imlec_y][imlec_x] = 'O';
          yapildi = 1;
        }

        if (yapildi) {
          int sonuc = kontrol_et();
          if (sonuc != 0) {
            oyun_bitti = 1;
            kazanan = sonuc;
            if (sonuc == 1)
              skor_x++;
            else if (sonuc == 2)
              skor_o++;
            else
              beraberlik++;
          } else {
            sira = !sira;
          }
        }
      }
      ekrani_ciz();
    }
  }
}
