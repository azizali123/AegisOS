/* ============================================================
   pentomino.c - AegisOS v4.0 Pentomino Challenge
   Tetris'in 5 kareli zorlu atasi
   12 farkli sekil, 10x20 oyun alani
   ============================================================ */
#include "../include/keyboard.h"
#include "../include/vga.h"
#include <stdint.h>

/* ---- Oyun Alani ---- */
#define PW 10      /* Genislik */
#define PH 20      /* Yukseklik */
#define P_SCALE 2  /* Her kare 2 karakter */

/* Cerceve konumlari */
#define P_WX 28
#define P_WY 4

/* Parca boyutu: 5x5 matris (pentomino 5 kare) */
#define P_SIZE 5

/* ---- Global Degiskenler ---- */
static int p_board[PH][PW];
static int p_piece[P_SIZE][P_SIZE];
static int p_px, p_py;
static int p_cur_type, p_next_type;
static int p_score, p_level, p_lines, p_over;

/* ---- 12 Pentomino Sekli ---- */
/* F, I, L, N, P, T, U, V, W, X, Y, Z */
static const int pentominoes[12][P_SIZE][P_SIZE] = {
    /* F */
    {{0,1,1,0,0},
     {1,1,0,0,0},
     {0,1,0,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0}},
    /* I */
    {{1,1,1,1,1},
     {0,0,0,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0}},
    /* L */
    {{1,0,0,0,0},
     {1,0,0,0,0},
     {1,0,0,0,0},
     {1,1,0,0,0},
     {0,0,0,0,0}},
    /* N */
    {{0,1,0,0,0},
     {1,1,0,0,0},
     {1,0,0,0,0},
     {1,0,0,0,0},
     {0,0,0,0,0}},
    /* P */
    {{1,1,0,0,0},
     {1,1,0,0,0},
     {1,0,0,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0}},
    /* T */
    {{1,1,1,0,0},
     {0,1,0,0,0},
     {0,1,0,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0}},
    /* U */
    {{1,0,1,0,0},
     {1,1,1,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0}},
    /* V */
    {{1,0,0,0,0},
     {1,0,0,0,0},
     {1,1,1,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0}},
    /* W */
    {{1,0,0,0,0},
     {1,1,0,0,0},
     {0,1,1,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0}},
    /* X */
    {{0,1,0,0,0},
     {1,1,1,0,0},
     {0,1,0,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0}},
    /* Y */
    {{0,1,0,0,0},
     {1,1,0,0,0},
     {0,1,0,0,0},
     {0,1,0,0,0},
     {0,0,0,0,0}},
    /* Z */
    {{1,1,0,0,0},
     {0,1,0,0,0},
     {0,1,1,0,0},
     {0,0,0,0,0},
     {0,0,0,0,0}}
};

/* Renkler (12 farkli) */
static const uint8_t p_colors[12] = {
    0x0C, 0x0B, 0x06, 0x0D, 0x0A, 0x0E,
    0x09, 0x02, 0x05, 0x0F, 0x03, 0x04
};

/* ---- Rastgele ---- */
static uint32_t p_seed = 54321;
static int p_rand(void) {
    p_seed = p_seed * 1103515245 + 12345;
    return (p_seed >> 16) & 0x7FFF;
}

/* ---- Parca Islemleri ---- */
static void p_copy(int dst[P_SIZE][P_SIZE], const int src[P_SIZE][P_SIZE]) {
    for (int i = 0; i < P_SIZE; i++)
        for (int j = 0; j < P_SIZE; j++)
            dst[i][j] = src[i][j];
}

static void p_load(int type) {
    p_cur_type = type % 12;
    p_copy(p_piece, pentominoes[p_cur_type]);
    p_px = PW / 2 - 2;
    p_py = -1;
}

static int p_hit(int nx, int ny, int p[P_SIZE][P_SIZE]) {
    for (int i = 0; i < P_SIZE; i++)
        for (int j = 0; j < P_SIZE; j++)
            if (p[i][j]) {
                int bx = nx + j, by = ny + i;
                if (bx < 0 || bx >= PW || by >= PH) return 1;
                if (by >= 0 && p_board[by][bx]) return 1;
            }
    return 0;
}

static void p_rotate(void) {
    int tmp[P_SIZE][P_SIZE];
    for (int i = 0; i < P_SIZE; i++)
        for (int j = 0; j < P_SIZE; j++)
            tmp[i][j] = p_piece[P_SIZE - 1 - j][i];
    if (!p_hit(p_px, p_py, tmp))
        p_copy(p_piece, tmp);
}

static void p_merge(void) {
    for (int i = 0; i < P_SIZE; i++)
        for (int j = 0; j < P_SIZE; j++)
            if (p_piece[i][j]) {
                int by = p_py + i, bx = p_px + j;
                if (by >= 0 && by < PH && bx >= 0 && bx < PW)
                    p_board[by][bx] = p_cur_type + 1;
            }
}

static void p_clear_lines(void) {
    int cl = 0;
    for (int y = PH - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < PW; x++)
            if (!p_board[y][x]) full = 0;
        if (full) {
            cl++;
            for (int yy = y; yy > 0; yy--)
                for (int x = 0; x < PW; x++)
                    p_board[yy][x] = p_board[yy - 1][x];
            for (int x = 0; x < PW; x++)
                p_board[0][x] = 0;
            y++; /* ayni satiri tekrar kontrol et */
        }
    }
    if (cl) {
        p_lines += cl;
        /* Pentomino bonusu: 5 satir birden = PENTOMINO */
        if (cl >= 5)
            p_score += 5000;
        else
            p_score += cl * cl * 150;
        p_level = p_lines / 8;
    }
}

/* ---- Cizim ---- */
static void p_draw_border(void) {
    vga_clear(0x00);

    /* Dis cerceve */
    int cerceve_h = 30;
    for (int x = 0; x < 80; x++) {
        vga_set_cursor(x, 0); vga_putc('*', 0x05);
        vga_set_cursor(x, cerceve_h); vga_putc('*', 0x05);
    }
    for (int y = 0; y <= cerceve_h; y++) {
        vga_set_cursor(0, y);  vga_puts("||", 0x05);
        vga_set_cursor(78, y); vga_puts("||", 0x05);
    }

    /* Ust baslik */
    vga_set_cursor(2, 1); vga_putc('+', 0x0D);
    for (int i = 0; i < 74; i++) vga_putc('=', 0x0D);
    vga_putc('+', 0x0D);

    vga_set_cursor(2, 2); vga_putc('|', 0x0D);
    vga_set_cursor(77, 2); vga_putc('|', 0x0D);

    vga_set_cursor(2, 3); vga_putc('+', 0x0D);
    for (int i = 0; i < 74; i++) vga_putc('=', 0x0D);
    vga_putc('+', 0x0D);

    vga_set_cursor(28, 2);
    vga_puts(">>> PENTOMINO CHALLENGE <<<", 0x0E);

    /* Oyun alani duvarlari */
    for (int y = P_WY; y < P_WY + PH; y++) {
        vga_set_cursor(P_WX - 2, y); vga_putc('|', 0x0F);
        vga_set_cursor(P_WX + PW * P_SCALE, y); vga_putc('|', 0x0F);
    }

    /* Alt bilgi */
    int alt_y = P_WY + PH + 1;
    vga_set_cursor(2, alt_y); vga_putc('+', 0x0D);
    for (int i = 0; i < 74; i++) vga_putc('=', 0x0D);
    vga_putc('+', 0x0D);

    vga_set_cursor(4, alt_y + 1);
    vga_puts("OKLAR: YON/CEVIR  SPACE: SERT DUSUR  F1: YENI OYUN  ESC: CIK", 0x0F);

    /* Sag bilgi paneli */
    int ix = P_WX + (PW * P_SCALE) + 3;
    vga_set_cursor(ix, P_WY + 1);
    vga_puts("[ ISTATISTIK ]", 0x0E);
    vga_set_cursor(ix, P_WY + 3);
    vga_puts("SKOR:", 0x07);
    vga_set_cursor(ix, P_WY + 6);
    vga_puts("SEVIYE:", 0x07);
    vga_set_cursor(ix, P_WY + 9);
    vga_puts("SATIR:", 0x07);
    vga_set_cursor(ix, P_WY + 12);
    vga_puts("SONRAKI:", 0x07);

    /* Sol bilgi paneli */
    int lx = 4;
    vga_set_cursor(lx, P_WY + 1);
    vga_puts("[ BILGI ]", 0x0E);
    vga_set_cursor(lx, P_WY + 3);
    vga_puts("12 Farkli", 0x07);
    vga_set_cursor(lx, P_WY + 4);
    vga_puts("5-Kareli", 0x07);
    vga_set_cursor(lx, P_WY + 5);
    vga_puts("Sekil!", 0x07);
    vga_set_cursor(lx, P_WY + 7);
    vga_puts("5 Satir =", 0x0D);
    vga_set_cursor(lx, P_WY + 8);
    vga_puts("PENTOMINO!", 0x0D);
}

static void p_draw(void) {
    int ix = P_WX + (PW * P_SCALE) + 3;
    char b[12];
    int s, k;

    /* Skor */
    s = p_score; k = 0;
    if (s == 0) b[k++] = '0';
    else while (s > 0) { b[k++] = (char)('0' + (s % 10)); s /= 10; }
    b[k] = 0;
    for (int i = 0; i < k / 2; i++) { char t = b[i]; b[i] = b[k-1-i]; b[k-1-i] = t; }
    vga_set_cursor(ix, P_WY + 4);
    vga_puts("        ", 0x00);
    vga_set_cursor(ix, P_WY + 4);
    vga_puts(b, 0x0E);

    /* Seviye */
    s = p_level; k = 0;
    if (s == 0) b[k++] = '0';
    else while (s > 0) { b[k++] = (char)('0' + (s % 10)); s /= 10; }
    b[k] = 0;
    for (int i = 0; i < k / 2; i++) { char t = b[i]; b[i] = b[k-1-i]; b[k-1-i] = t; }
    vga_set_cursor(ix, P_WY + 7);
    vga_puts("        ", 0x00);
    vga_set_cursor(ix, P_WY + 7);
    vga_puts(b, 0x0E);

    /* Satir */
    s = p_lines; k = 0;
    if (s == 0) b[k++] = '0';
    else while (s > 0) { b[k++] = (char)('0' + (s % 10)); s /= 10; }
    b[k] = 0;
    for (int i = 0; i < k / 2; i++) { char t = b[i]; b[i] = b[k-1-i]; b[k-1-i] = t; }
    vga_set_cursor(ix, P_WY + 10);
    vga_puts("        ", 0x00);
    vga_set_cursor(ix, P_WY + 10);
    vga_puts(b, 0x0E);

    /* Tahta render */
    int display[PH][PW];
    for (int y = 0; y < PH; y++)
        for (int x = 0; x < PW; x++)
            display[y][x] = p_board[y][x];

    for (int i = 0; i < P_SIZE; i++)
        for (int j = 0; j < P_SIZE; j++)
            if (p_piece[i][j]) {
                int by = p_py + i, bx = p_px + j;
                if (by >= 0 && by < PH && bx >= 0 && bx < PW)
                    display[by][bx] = p_cur_type + 1;
            }

    for (int y = 0; y < PH; y++)
        for (int x = 0; x < PW; x++) {
            vga_set_cursor(P_WX + x * P_SCALE, P_WY + y);
            int val = display[y][x];
            if (val) {
                uint8_t c = p_colors[(val - 1) % 12];
                vga_putc('[', c);
                vga_putc(']', c);
            } else {
                vga_puts(" .", 0x08);
            }
        }

    /* Sonraki parca */
    int nx_x = ix;
    int nx_y = P_WY + 14;
    for (int i = 0; i < P_SIZE; i++) {
        vga_set_cursor(nx_x, nx_y + i);
        vga_puts("          ", 0x00);
    }
    for (int i = 0; i < P_SIZE; i++)
        for (int j = 0; j < P_SIZE; j++)
            if (pentominoes[p_next_type % 12][i][j]) {
                vga_set_cursor(nx_x + j * P_SCALE, nx_y + i);
                vga_putc('[', p_colors[p_next_type % 12]);
                vga_putc(']', p_colors[p_next_type % 12]);
            }

    vga_swap_buffers();
}

/* ---- Ana Oyun Dongusu ---- */
void game_pentomino(void) {
    /* Sifirla */
    for (int y = 0; y < PH; y++)
        for (int x = 0; x < PW; x++)
            p_board[y][x] = 0;
    p_score = 0; p_level = 0; p_lines = 0; p_over = 0;

    p_next_type = p_rand() % 12;
    p_load(p_rand() % 12);
    p_next_type = p_rand() % 12;
    int tick = 0;

    p_draw_border();
    vga_swap_buffers();

    /* Input temizle */
    while (input_available()) input_get();

    while (!p_over) {
        int speed = (30 - p_level * 2);
        if (speed < 4) speed = 4;

        for (int p = 0; p < 10; p++) {
            int drew = 0;
            while (input_available()) {
                key_event_t ev = input_get();
                if (!ev.pressed) continue;
                int k = ev.key;

                if (k == 27) { vga_clear(0x07); return; }

                if (k == KEY_F1) {
                    for (int y = 0; y < PH; y++)
                        for (int x = 0; x < PW; x++)
                            p_board[y][x] = 0;
                    p_score = 0; p_level = 0; p_lines = 0; p_over = 0;
                    p_load(p_rand() % 12);
                    p_next_type = p_rand() % 12;
                    p_draw_border();
                    vga_swap_buffers();
                }
                if ((k == KEY_LEFT  || k == 'a' || k == 'A') && !p_hit(p_px - 1, p_py, p_piece)) p_px--;
                if ((k == KEY_RIGHT || k == 'd' || k == 'D') && !p_hit(p_px + 1, p_py, p_piece)) p_px++;
                if ((k == KEY_UP    || k == 'w' || k == 'W')) p_rotate();
                if ((k == KEY_DOWN  || k == 's' || k == 'S') && !p_hit(p_px, p_py + 1, p_piece)) {
                    p_py++; p_score += 2;
                }
                /* Space: sert dusur */
                if (k == ' ') {
                    while (!p_hit(p_px, p_py + 1, p_piece)) { p_py++; p_score += 3; }
                }
                drew = 1;
            }
            if (drew) p_draw();
        }

        if (tick++ > speed) {
            tick = 0;
            if (!p_hit(p_px, p_py + 1, p_piece))
                p_py++;
            else {
                p_merge();
                p_clear_lines();
                p_load(p_next_type);
                p_next_type = p_rand() % 12;
                if (p_hit(p_px, p_py, p_piece))
                    p_over = 1;
            }
            p_draw();
        }
        for (volatile int d = 0; d < 3875000; d++) {
            if ((d & 0x7FF) == 0) keyboard_poll();
        }
    }

    /* Oyun Bitti */
    vga_set_cursor(28, 12);
    vga_puts("  OYUN BITTI!  ", 0x4F);
    vga_set_cursor(25, 14);
    vga_puts("    SKOR: ", 0x4E);
    vga_put_dec((uint32_t)p_score, 0x4E);
    vga_puts("    ", 0x4E);
    vga_swap_buffers();
    while (1) {
        key_event_t ev = input_get();
        if (ev.pressed && ev.key == 27) break;
    }
    vga_clear(0x07);
}
