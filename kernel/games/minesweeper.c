#include "../include/keyboard.h"
#include "../include/vga.h"
#include <stdint.h>

// MAYIN TARLASI (MINESWEEPER) - PACMAN STILI ARAYUZ

#define M_GENISLIK 16 // 16x11 (Kucuk ama yeterli)
#define M_YUKSEKLIK 11

#define P_X 10 // Offset X
#define P_Y 6  // Offset Y

static int tahta[M_YUKSEKLIK][M_GENISLIK];
static int durum[M_YUKSEKLIK][M_GENISLIK];
static int imlec_x = 0, imlec_y = 0;
static int oyun_bitti = 0;
static int skor = 0;
static int toplam_mayin = 0;

static uint32_t ms_seed = 123456;
static int m_rastgele(void) {
  ms_seed = ms_seed * 1103515245 + 12345;
  return (ms_seed >> 16) & 0x7FFF;
}

static void tahta_olustur(void) {
  for (int i = 0; i < M_YUKSEKLIK; i++) {
    for (int j = 0; j < M_GENISLIK; j++) {
      tahta[i][j] = 0;
      durum[i][j] = 0;
    }
  }

  toplam_mayin = 20;
  for (int k = 0; k < toplam_mayin; k++) {
    int r = m_rastgele() % M_YUKSEKLIK;
    int c = m_rastgele() % M_GENISLIK;
    if (tahta[r][c] != 9)
      tahta[r][c] = 9;
    else
      k--;
  }

  for (int i = 0; i < M_YUKSEKLIK; i++) {
    for (int j = 0; j < M_GENISLIK; j++) {
      if (tahta[i][j] == 9)
        continue;
      int n = 0;
      for (int di = -1; di <= 1; di++) {
        for (int dj = -1; dj <= 1; dj++) {
          int ni = i + di;
          int nj = j + dj;
          if (ni >= 0 && ni < M_YUKSEKLIK && nj >= 0 && nj < M_GENISLIK &&
              tahta[ni][nj] == 9)
            n++;
        }
      }
      tahta[i][j] = n;
    }
  }
}

static void hucre_tac(int r, int c) {
  if (r < 0 || r >= M_YUKSEKLIK || c < 0 || c >= M_GENISLIK ||
      durum[r][c] == 1 || durum[r][c] == 2)
    return;
  durum[r][c] = 1;
  skor += 10;
  if (tahta[r][c] == 0) {
    for (int di = -1; di <= 1; di++)
      for (int dj = -1; dj <= 1; dj++)
        hucre_tac(r + di, c + dj);
  }
}

static void cerceve_ciz_pacman_style(void) {
  vga_clear(0x00);
  // DIS CERCEVE
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

  vga_set_cursor(35, 2);
  vga_puts("MAYIN TARLASI", 0x0E);

  // ALT KUTU
  vga_set_cursor(2, 22);
  vga_putc('+', 0x0B);
  for (int i = 0; i < 74; i++)
    vga_putc('=', 0x0B);
  vga_putc('+', 0x0B);

  vga_set_cursor(4, 23);
  vga_puts("OKLAR: Gez   ENT/BOS: Ac   F: Bayrak   F1: Yeni", 0x0F);
}

static void ekrani_ciz(void) {
  int baslangic_x = P_X + 2;
  int baslangic_y = P_Y + 1;

  // Skor Yazdirma (Sag alt kosede)
  vga_set_cursor(60, 23);
  vga_puts("SKOR: ", 0x0B);
  char puan_str[10];
  int s = skor, k = 0;
  if (s == 0)
    puan_str[k++] = '0';
  else
    while (s > 0) {
      puan_str[k++] = (s % 10) + '0';
      s /= 10;
    }
  puan_str[k] = 0;
  for (int i = 0; i < k / 2; i++) {
    char t = puan_str[i];
    puan_str[i] = puan_str[k - 1 - i];
    puan_str[k - 1 - i] = t;
  }
  vga_puts(puan_str, 0x0F);

  // OYUN ALANI CIZIMI
  int oyun_y = baslangic_y;
  int oyun_x = baslangic_x + 5;

  for (int i = 0; i < M_YUKSEKLIK; i++) {
    vga_set_cursor(oyun_x, oyun_y + i);
    for (int j = 0; j < M_GENISLIK; j++) {
      int secili = (i == imlec_y && j == imlec_x);
      uint8_t renk = secili ? 0x70 : 0x08;
      if (durum[i][j] == 0) {
        vga_putc('[', renk);
        vga_putc(' ', renk);
        vga_putc(']', renk);
      } else if (durum[i][j] == 2) {
        vga_putc('[', renk);
        vga_putc('F', 0x0C | (secili ? 0x70 : 0));
        vga_putc(']', renk);
      } else {
        if (tahta[i][j] == 9) {
          vga_putc('[', 0x04);
          vga_putc('*', 0x0E);
          vga_putc(']', 0x04);
        } else if (tahta[i][j] == 0) {
          vga_putc('[', 0x08);
          vga_putc('.', 0x08);
          vga_putc(']', 0x08);
        } else {
          char num = tahta[i][j] + '0';
          uint8_t num_renk = 0x0F;
          if (tahta[i][j] == 1)
            num_renk = 0x09;
          if (tahta[i][j] == 2)
            num_renk = 0x0A;
          if (tahta[i][j] == 3)
            num_renk = 0x0C;
          vga_putc('[', 0x08);
          vga_putc(num, num_renk);
          vga_putc(']', 0x08);
        }
      }
    }
  }

  if (oyun_bitti) {
    vga_set_cursor(35, 12);
    vga_puts(" OYUN BITTI ", 0x4F);
  }
}

void game_minesweeper(void) {
yeniden_baslat:
  skor = 0;
  imlec_x = M_GENISLIK / 2;
  imlec_y = M_YUKSEKLIK / 2;
  oyun_bitti = 0;

  tahta_olustur();
  cerceve_ciz_pacman_style();
  ekrani_ciz();

  while (keyboard_check_key())
    ;

  while (1) {
    int tus = keyboard_read_key();
    if (tus == 27) {
      vga_clear(0x07);
      return;
    }
    if (tus == KEY_F1)
      goto yeniden_baslat;

    if (!oyun_bitti) {
      if (tus == KEY_LEFT && imlec_x > 0)
        imlec_x--;
      if (tus == KEY_RIGHT && imlec_x < M_GENISLIK - 1)
        imlec_x++;
      if (tus == KEY_UP && imlec_y > 0)
        imlec_y--;
      if (tus == KEY_DOWN && imlec_y < M_YUKSEKLIK - 1)
        imlec_y++;

      if (tus == 'f' || tus == 'F') {
        if (durum[imlec_y][imlec_x] == 0)
          durum[imlec_y][imlec_x] = 2;
        else if (durum[imlec_y][imlec_x] == 2)
          durum[imlec_y][imlec_x] = 0;
      }

      if (tus == '\n' || tus == ' ') {
        if (durum[imlec_y][imlec_x] != 2 && durum[imlec_y][imlec_x] == 0) {
          if (tahta[imlec_y][imlec_x] == 9) {
            oyun_bitti = 1;
            for (int i = 0; i < M_YUKSEKLIK; i++)
              for (int j = 0; j < M_GENISLIK; j++)
                if (tahta[i][j] == 9)
                  durum[i][j] = 1;
          } else {
            hucre_tac(imlec_y, imlec_x);
          }
        }
      }
      ekrani_ciz();
    }
  }
}
