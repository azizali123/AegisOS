#include "../include/keyboard.h"
#include "../include/vga.h"
#include "../include/pit.h"
#include <stdint.h>

/* ============================================================
   PACMAN OYUNU - AegisOS v3.0
   Sabit labirent, 4 hayalet (Blinky/Pinky/Inky/Clyde),
   Durum makinesi, guc toplari, zaman tabanli hareket
   ============================================================ */

/* Oyun alani sabitleri */
#define MAP_W 28
#define MAP_H 18
#define OFF_X 26    /* Ekranda yatay offset (ortala) */
#define OFF_Y 4     /* Ekranda dikey offset */

/* Hayalet durumlari */
#define GHOST_SCATTER    0
#define GHOST_CHASE      1
#define GHOST_FRIGHTENED 2
#define GHOST_EYES       3

/* Zamanlama (ms) */
#define PACMAN_MOVE_MS    150
#define GHOST_MOVE_MS     200
#define GHOST_FRIGHT_MS   180   /* Korkmus hayalet yavasi */
#define FRIGHT_DURATION   6000  /* Korkmus mod suresi */
#define SCATTER_DURATION  7000
#define CHASE_DURATION    20000

/* Yonler */
#define DIR_NONE  0
#define DIR_UP    1
#define DIR_DOWN  2
#define DIR_LEFT  3
#define DIR_RIGHT 4

/* RNG */
static uint32_t pm_seed = 84723;
static int pm_rand(void) {
    pm_seed = pm_seed * 1103515245 + 12345;
    return (pm_seed >> 16) & 0x7FFF;
}
static int pm_abs(int x) { return x < 0 ? -x : x; }

/* Sabit Pacman haritasi - klasik gorunume yakin */
/* 0:bos, 1:duvar, 2:yem(nokta), 3:guc topu, 4:hayalet evi, 5:kapi */
static const char base_map[MAP_H][MAP_W+1] = {
    "1111111111111111111111111111",
    "1222222122222112222212222221",
    "1311121121111112111121211131",
    "1222222222222222222222222221",
    "1211121121111111121121211121",
    "1222221122221112222112222221",
    "1111121111110110111112111111",
    "0000021100044440011210000000",
    "1111121100041440011211111111",
    "0000022200040040022200000000",
    "1111121101111111101211111111",
    "1222222100000000012222222221",
    "1211121111121112111121211121",
    "1322121222222222222212212231",
    "1112121121111111121121212111",
    "1222222122221112222212222221",
    "1211111111111111111111111121",
    "1111111111111111111111111111"
};

static char map[MAP_H][MAP_W];

/* Oyuncu durumu */
static int pac_x, pac_y;
static int pac_dir;        /* Mevcut yon */
static int pac_next_dir;   /* Sira bekleyen yon (queued turn) */
static int score;
static int level;
static int lives;
static int total_pellets;
static int eaten_pellets;
static int game_over;
static int game_won;

/* Hayalet yapisi */
typedef struct {
    int x, y;
    int dir;
    int state;        /* SCATTER, CHASE, FRIGHTENED, EYES */
    int home_x, home_y; /* Scatter hedef kosesi */
    char symbol;
    uint8_t color;
    uint8_t fright_color;
} Ghost;

#define GHOST_COUNT 4
static Ghost ghosts[GHOST_COUNT];

/* Durum zamanlayicilari */
static uint64_t fright_start;
static uint64_t mode_start;
static int global_mode; /* 0=scatter, 1=chase */

/* Skor bonus */
static int ghost_eat_combo; /* Ayni fright'ta yenen hayalet sayisi */

/* ---- Harita Islemleri ---- */
static void init_map(void) {
    total_pellets = 0;
    eaten_pellets = 0;
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            map[y][x] = base_map[y][x] - '0';
            if (map[y][x] == 2 || map[y][x] == 3)
                total_pellets++;
        }
    }
}

static int is_walkable(int x, int y) {
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) return 0;
    return map[y][x] != 1;
}

static int is_ghost_walkable(int x, int y) {
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) return 0;
    int tile = map[y][x];
    return tile != 1; /* Hayaletler kapi ve evden gecebilir */
}

/* ---- Yonetim Fonksiyonlari ---- */
static void get_dir_delta(int dir, int *dx, int *dy) {
    *dx = 0; *dy = 0;
    if (dir == DIR_UP)    *dy = -1;
    if (dir == DIR_DOWN)  *dy = 1;
    if (dir == DIR_LEFT)  *dx = -1;
    if (dir == DIR_RIGHT) *dx = 1;
}

/* ---- UI Cizimi ---- */
static void draw_ui_frame(void) {
    vga_clear(0x00);

    /* Dis cerceve */
    for (int x = 0; x < 80; x++) {
        vga_set_cursor(x, 0); vga_putc('*', 0x09);
        vga_set_cursor(x, 24); vga_putc('*', 0x09);
    }
    for (int y = 0; y < 25; y++) {
        vga_set_cursor(0, y); vga_puts("||", 0x09);
        vga_set_cursor(78, y); vga_puts("||", 0x09);
    }

    /* Ust kutu */
    vga_set_cursor(2, 1); vga_putc('+', 0x0B);
    for (int i = 0; i < 74; i++) vga_putc('=', 0x0B);
    vga_putc('+', 0x0B);

    vga_set_cursor(2, 2); vga_putc('|', 0x0B);
    vga_set_cursor(77, 2); vga_putc('|', 0x0B);

    vga_set_cursor(2, 3); vga_putc('+', 0x0B);
    for (int i = 0; i < 74; i++) vga_putc('=', 0x0B);
    vga_putc('+', 0x0B);

    /* Baslik */
    vga_set_cursor(33, 2);
    vga_puts("PACMAN  OYUNU", 0x0E);

    /* Skor ve seviye */
    vga_set_cursor(4, 2);
    vga_puts("SKOR:", 0x0B);
    vga_put_dec((uint32_t)score, 0x0F);

    vga_set_cursor(60, 2);
    vga_puts("SEVIYE:", 0x0B);
    vga_put_dec((uint32_t)level, 0x0F);

    vga_set_cursor(70, 2);
    vga_puts("CAN:", 0x0B);
    vga_put_dec((uint32_t)lives, 0x0F);

    /* Oyun alani cercevesi */
    for (int y = OFF_Y; y < OFF_Y + MAP_H; y++) {
        vga_set_cursor(OFF_X - 1, y); vga_putc('|', 0x08);
        vga_set_cursor(OFF_X + MAP_W, y); vga_putc('|', 0x08);
    }

    /* Alt kutu */
    vga_set_cursor(2, 22); vga_putc('+', 0x0B);
    for (int i = 0; i < 74; i++) vga_putc('=', 0x0B);
    vga_putc('+', 0x0B);

    vga_set_cursor(4, 23);
    vga_puts("ESC:CIKIS  OKLAR/WASD:YON  ENTER:YENI OYUN  P:DURAKLAT", 0x0F);
}

static void update_score_display(void) {
    vga_set_cursor(9, 2);
    vga_puts("     ", 0x00); /* Temizle */
    vga_set_cursor(9, 2);
    vga_put_dec((uint32_t)score, 0x0F);
}

/* Haritayi ciz */
static void draw_map(void) {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            vga_set_cursor(OFF_X + x, OFF_Y + y);
            int tile = map[y][x];
            if (tile == 1)
                vga_putc(0xDB, 0x01); /* Koyu mavi duvar */
            else if (tile == 2)
                vga_putc('.', 0x0E);  /* Sari yem noktasi */
            else if (tile == 3)
                vga_putc('O', 0x0F); /* Beyaz guc topu */
            else if (tile == 4)
                vga_putc(' ', 0x00); /* Hayalet evi */
            else if (tile == 5)
                vga_putc('-', 0x0D); /* Kapi */
            else
                vga_putc(' ', 0x00);
        }
    }
}

static void draw_entity(int x, int y, char c, uint8_t color) {
    vga_set_cursor(OFF_X + x, OFF_Y + y);
    vga_putc(c, color);
}

/* ---- Hayalet AI ---- */
static int distance_sq(int x1, int y1, int x2, int y2) {
    int dx = x1 - x2;
    int dy = y1 - y2;
    return dx*dx + dy*dy;
}

static int opposite_dir(int dir) {
    if (dir == DIR_UP)    return DIR_DOWN;
    if (dir == DIR_DOWN)  return DIR_UP;
    if (dir == DIR_LEFT)  return DIR_RIGHT;
    if (dir == DIR_RIGHT) return DIR_LEFT;
    return DIR_NONE;
}

/* Hayalet icin hedef noktaya en yakin yonu bul */
static int ghost_best_dir(Ghost *g, int target_x, int target_y) {
    int best_dir = g->dir;
    int best_dist = 999999;
    int opp = opposite_dir(g->dir);
    int dirs[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

    for (int i = 0; i < 4; i++) {
        if (dirs[i] == opp) continue; /* Geri donme */
        int dx, dy;
        get_dir_delta(dirs[i], &dx, &dy);
        int nx = g->x + dx;
        int ny = g->y + dy;
        if (!is_ghost_walkable(nx, ny)) continue;
        int dist = distance_sq(nx, ny, target_x, target_y);
        if (dist < best_dist) {
            best_dist = dist;
            best_dir = dirs[i];
        }
    }
    return best_dir;
}

static void ghost_get_target(Ghost *g, int idx, int *tx, int *ty) {
    if (g->state == GHOST_SCATTER) {
        *tx = g->home_x;
        *ty = g->home_y;
        return;
    }
    if (g->state == GHOST_EYES) {
        /* Merkeze don */
        *tx = MAP_W / 2;
        *ty = MAP_H / 2 - 1;
        return;
    }
    if (g->state == GHOST_FRIGHTENED) {
        /* Rastgele */
        *tx = pm_rand() % MAP_W;
        *ty = pm_rand() % MAP_H;
        return;
    }

    /* CHASE modu */
    switch (idx) {
        case 0: /* Blinky - dogrudan Pacman'i takip et */
            *tx = pac_x; *ty = pac_y;
            break;
        case 1: /* Pinky - Pacman'in 4 kare onunu hedefle */
        {
            int dx, dy;
            get_dir_delta(pac_dir, &dx, &dy);
            *tx = pac_x + dx * 4;
            *ty = pac_y + dy * 4;
            break;
        }
        case 2: /* Inky - Blinky'den yansima */
        {
            int dx, dy;
            get_dir_delta(pac_dir, &dx, &dy);
            int ax = pac_x + dx * 2;
            int ay = pac_y + dy * 2;
            *tx = 2 * ax - ghosts[0].x;
            *ty = 2 * ay - ghosts[0].y;
            break;
        }
        case 3: /* Clyde - yakinsa scatter, uzaksa chase */
        {
            int dist = distance_sq(g->x, g->y, pac_x, pac_y);
            if (dist < 64) {
                *tx = g->home_x; *ty = g->home_y;
            } else {
                *tx = pac_x; *ty = pac_y;
            }
            break;
        }
    }
}

/* ---- Oyun Durumu Baslatma ---- */
static void init_game_state(void) {
    init_map();
    pac_x = MAP_W / 2;
    pac_y = MAP_H - 3;
    pac_dir = DIR_LEFT;
    pac_next_dir = DIR_NONE;
    score = 0;
    lives = 3;
    game_over = 0;
    game_won = 0;
    ghost_eat_combo = 0;
    global_mode = GHOST_SCATTER;
    mode_start = pit_get_millis();

    /* Blinky (Kirmizi) */
    ghosts[0] = (Ghost){ MAP_W/2, MAP_H/2-2, DIR_LEFT, GHOST_SCATTER,
                          MAP_W-1, 0, 'M', 0x0C, 0x01 };
    /* Pinky (Pembe) */
    ghosts[1] = (Ghost){ MAP_W/2-1, MAP_H/2-1, DIR_UP, GHOST_SCATTER,
                          0, 0, 'M', 0x0D, 0x01 };
    /* Inky (Cyan) */
    ghosts[2] = (Ghost){ MAP_W/2, MAP_H/2-1, DIR_DOWN, GHOST_SCATTER,
                          MAP_W-1, MAP_H-1, 'M', 0x0B, 0x01 };
    /* Clyde (Turuncu) */
    ghosts[3] = (Ghost){ MAP_W/2+1, MAP_H/2-1, DIR_UP, GHOST_SCATTER,
                          0, MAP_H-1, 'M', 0x06, 0x01 };
}

static void reset_positions(void) {
    pac_x = MAP_W / 2;
    pac_y = MAP_H - 3;
    pac_dir = DIR_LEFT;
    pac_next_dir = DIR_NONE;

    ghosts[0].x = MAP_W/2;   ghosts[0].y = MAP_H/2-2;
    ghosts[1].x = MAP_W/2-1; ghosts[1].y = MAP_H/2-1;
    ghosts[2].x = MAP_W/2;   ghosts[2].y = MAP_H/2-1;
    ghosts[3].x = MAP_W/2+1; ghosts[3].y = MAP_H/2-1;

    for (int i = 0; i < GHOST_COUNT; i++) {
        ghosts[i].state = GHOST_SCATTER;
        ghosts[i].dir = DIR_UP;
    }
}

/* ============================================================
   ANA OYUN DONGUSU
   ============================================================ */
void game_pacman(void) {
start_game:
    init_game_state();
    level = 1;

new_level:
    draw_ui_frame();
    draw_map();

    /* Pacman ciz */
    draw_entity(pac_x, pac_y, 'C', 0x0E);

    /* Hayaletleri ciz */
    for (int i = 0; i < GHOST_COUNT; i++)
        draw_entity(ghosts[i].x, ghosts[i].y, ghosts[i].symbol, ghosts[i].color);

    vga_swap_buffers();

    /* Input buffer temizle */
    while (input_available()) input_get();

    uint64_t last_pac_move   = pit_get_millis();
    uint64_t last_ghost_move = pit_get_millis();
    mode_start = pit_get_millis();
    fright_start = 0;

    while (!game_over && !game_won) {
        uint64_t now = pit_get_millis();

        /* ---- INPUT ---- */
        while (input_available()) {
            key_event_t ev = input_get();
            if (!ev.pressed) continue;
            int k = ev.key;

            if (k == 27) { vga_clear(0x07); return; }
            if (k == '\n') goto start_game;

            if (k == KEY_UP    || k == 'W' || k == 'w') pac_next_dir = DIR_UP;
            if (k == KEY_DOWN  || k == 'S' || k == 's') pac_next_dir = DIR_DOWN;
            if (k == KEY_LEFT  || k == 'A' || k == 'a') pac_next_dir = DIR_LEFT;
            if (k == KEY_RIGHT || k == 'D' || k == 'd') pac_next_dir = DIR_RIGHT;
        }

        /* ---- GLOBAL MOD GECISI ---- */
        if (now - mode_start >= (uint64_t)(global_mode == 0 ? SCATTER_DURATION : CHASE_DURATION)) {
            global_mode = 1 - global_mode;
            mode_start = now;
            for (int i = 0; i < GHOST_COUNT; i++) {
                if (ghosts[i].state != GHOST_FRIGHTENED && ghosts[i].state != GHOST_EYES)
                    ghosts[i].state = global_mode;
            }
        }

        /* Frightened zamanlayici */
        if (fright_start > 0 && (now - fright_start >= FRIGHT_DURATION)) {
            fright_start = 0;
            ghost_eat_combo = 0;
            for (int i = 0; i < GHOST_COUNT; i++) {
                if (ghosts[i].state == GHOST_FRIGHTENED)
                    ghosts[i].state = global_mode;
            }
        }

        /* ---- PACMAN HAREKETI ---- */
        if (now - last_pac_move >= PACMAN_MOVE_MS) {
            last_pac_move = now;

            /* Queued turn: istenen yon uygunsa degistir */
            if (pac_next_dir != DIR_NONE) {
                int dx, dy;
                get_dir_delta(pac_next_dir, &dx, &dy);
                if (is_walkable(pac_x + dx, pac_y + dy)) {
                    pac_dir = pac_next_dir;
                    pac_next_dir = DIR_NONE;
                }
            }

            /* Mevcut yonde ilerle */
            int dx, dy;
            get_dir_delta(pac_dir, &dx, &dy);
            int nx = pac_x + dx;
            int ny = pac_y + dy;

            if (is_walkable(nx, ny)) {
                /* Eski konumu temizle */
                draw_entity(pac_x, pac_y, ' ', 0x00);

                pac_x = nx;
                pac_y = ny;

                /* Yem yeme */
                if (map[pac_y][pac_x] == 2) {
                    score += 10;
                    map[pac_y][pac_x] = 0;
                    eaten_pellets++;
                    update_score_display();
                }
                /* Guc topu */
                else if (map[pac_y][pac_x] == 3) {
                    score += 50;
                    map[pac_y][pac_x] = 0;
                    eaten_pellets++;
                    ghost_eat_combo = 0;
                    fright_start = now;
                    for (int i = 0; i < GHOST_COUNT; i++) {
                        if (ghosts[i].state != GHOST_EYES) {
                            ghosts[i].state = GHOST_FRIGHTENED;
                            ghosts[i].dir = opposite_dir(ghosts[i].dir);
                        }
                    }
                    update_score_display();
                }

                /* Seviye tamamlandi mi? */
                if (eaten_pellets >= total_pellets) {
                    game_won = 1;
                }

                /* Yeni konumu ciz */
                char pc = 'C';
                if (pac_dir == DIR_LEFT) pc = '>';
                else if (pac_dir == DIR_RIGHT) pc = '<';
                else if (pac_dir == DIR_UP) pc = 'V';
                else if (pac_dir == DIR_DOWN) pc = '^';
                else pc = 'C';
                draw_entity(pac_x, pac_y, pc, 0x0E);
            }
        }

        /* ---- HAYALET HAREKETI ---- */
        uint64_t ghost_ms = GHOST_MOVE_MS;
        if (now - last_ghost_move >= ghost_ms) {
            last_ghost_move = now;

            for (int i = 0; i < GHOST_COUNT; i++) {
                Ghost *g = &ghosts[i];

                /* Eski pozisyonu temizle */
                int old_tile = map[g->y][g->x];
                if (old_tile == 2)
                    draw_entity(g->x, g->y, '.', 0x0E);
                else if (old_tile == 3)
                    draw_entity(g->x, g->y, 'O', 0x0F);
                else
                    draw_entity(g->x, g->y, ' ', 0x00);

                /* Hedef belirle ve yon sec */
                int tx, ty;
                ghost_get_target(g, i, &tx, &ty);

                if (g->state == GHOST_FRIGHTENED) {
                    /* Rastgele yon (geri donme yasak) */
                    int opp = opposite_dir(g->dir);
                    int tried = 0;
                    int new_dir = g->dir;
                    for (int t = 0; t < 8; t++) {
                        int r = (pm_rand() % 4);
                        int dirs[] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
                        int d = dirs[r];
                        if (d == opp) continue;
                        int ddx, ddy;
                        get_dir_delta(d, &ddx, &ddy);
                        if (is_ghost_walkable(g->x + ddx, g->y + ddy)) {
                            new_dir = d;
                            tried = 1;
                            break;
                        }
                    }
                    if (!tried) new_dir = ghost_best_dir(g, tx, ty);
                    g->dir = new_dir;
                } else if (g->state == GHOST_EYES) {
                    /* Merkeze don, varildiginda scatter'a gec */
                    g->dir = ghost_best_dir(g, tx, ty);
                    if (g->x == MAP_W/2 && g->y == MAP_H/2-1) {
                        g->state = global_mode;
                    }
                } else {
                    g->dir = ghost_best_dir(g, tx, ty);
                }

                /* Hareket et */
                int gdx, gdy;
                get_dir_delta(g->dir, &gdx, &gdy);
                int gnx = g->x + gdx;
                int gny = g->y + gdy;
                if (is_ghost_walkable(gnx, gny)) {
                    g->x = gnx;
                    g->y = gny;
                }

                /* Hayaleti ciz */
                uint8_t gc;
                char gs = g->symbol;
                if (g->state == GHOST_FRIGHTENED) {
                    gc = 0x01; /* Mavi korkmus */
                    gs = 'W';
                    /* Son 2 saniye yanip sonme */
                    if (fright_start > 0 && (now - fright_start > FRIGHT_DURATION - 2000)) {
                        if ((now / 250) % 2 == 0)
                            gc = 0x0F; /* Beyaz yanip sonme */
                    }
                } else if (g->state == GHOST_EYES) {
                    gc = 0x08;
                    gs = '"'; /* Goz */
                } else {
                    gc = g->color;
                }
                draw_entity(g->x, g->y, gs, gc);

                /* Pacman carpismasi */
                if (g->x == pac_x && g->y == pac_y) {
                    if (g->state == GHOST_FRIGHTENED) {
                        /* Hayalet yendi */
                        ghost_eat_combo++;
                        int bonus = 200;
                        for (int b = 1; b < ghost_eat_combo; b++) bonus *= 2;
                        score += bonus;
                        g->state = GHOST_EYES;
                        update_score_display();
                    } else if (g->state != GHOST_EYES) {
                        /* Pacman oldu */
                        lives--;
                        if (lives <= 0) {
                            game_over = 1;
                        } else {
                            /* Pozisyonlari sifirla, kisa bekleme */
                            draw_entity(pac_x, pac_y, 'X', 0x0C);
                            vga_swap_buffers();
                            pit_sleep_ms(1000);
                            reset_positions();
                            draw_ui_frame();
                            draw_map();
                            draw_entity(pac_x, pac_y, 'C', 0x0E);
                            for (int j = 0; j < GHOST_COUNT; j++)
                                draw_entity(ghosts[j].x, ghosts[j].y, ghosts[j].symbol, ghosts[j].color);
                            update_score_display();
                        }
                        break; /* Bu turda diger hayaletleri isleme */
                    }
                }
            }
        }

        vga_swap_buffers();

        /* Kucuk bekleme */
        for (volatile int d = 0; d < 200; d++)
            __asm__ volatile("pause");
    }

    /* Seviye gecisi */
    if (game_won) {
        vga_set_cursor(30, 12);
        vga_puts(" SEVIYE TAMAMLANDI! ", 0x2E);
        vga_swap_buffers();
        pit_sleep_ms(2000);
        level++;
        init_map();
        reset_positions();
        game_won = 0;
        goto new_level;
    }

    /* Game Over Mesaji */
    vga_set_cursor(32, 11);
    vga_puts("  OYUN BITTI!  ", 0x4F);
    vga_set_cursor(28, 13);
    vga_puts("  SKOR: ", 0x0E);
    vga_put_dec((uint32_t)score, 0x0F);
    vga_puts("  SEVIYE: ", 0x0E);
    vga_put_dec((uint32_t)level, 0x0F);
    vga_puts("  ", 0x0E);
    vga_swap_buffers();

    while (1) {
        key_event_t ev = input_get();
        if (!ev.pressed) continue;
        int k = ev.key;
        if (k == 27) { vga_clear(0x07); return; }
        if (k == '\n') goto start_game;
    }
}
