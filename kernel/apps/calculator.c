#include "../include/filesystem.h"
#include "../include/string.h"
#include "../include/input.h"
#include "../include/vga.h"
#include "../include/tui.h"
#include <stdint.h>

/* ============================================================
   AegisOS v4.0 - Aegis MATLAB (Bilimsel Analiz Merkezi)
   Gelistirilmis TUI Grafik ve Komut Satiri (REPL) Arayuzu
   ============================================================ */

/* FPU (Matematik Islemcisi) Yardimcilari */
static void fpu_init(void) { asm volatile("finit"); }
static double f_sin(double x) {
    double r;
    asm volatile("fsin" : "=t"(r) : "0"(x));
    return r;
}
static double f_cos(double x) {
    double r;
    asm volatile("fcos" : "=t"(r) : "0"(x));
    return r;
}
static double f_sqrt(double x) {
    double r;
    asm volatile("fsqrt" : "=t"(r) : "0"(x));
    return r;
}
static double f_yl2x(double y, double x) {
    double r;
    asm volatile("fyl2x" : "=t"(r) : "0"(x), "u"(y));
    return r;
}
static double f_2xm1(double x) {
    double r;
    asm volatile("f2xm1" : "=t"(r) : "0"(x));
    return r;
}
static double f_scale(double v, double s) {
    double r;
    asm volatile("fscale" : "=t"(r) : "0"(v), "u"(s));
    return r;
}
static double f_log(double x) {
    if (x <= 0) return 0.0;
    return f_yl2x(1.0, x) * 0.30102999566;
}
static double f_ln(double x) {
    if (x <= 0) return 0.0;
    return f_yl2x(1.0, x) * 0.69314718056;
}
static double f_tan(double x) {
    double s = f_sin(x);
    double c = f_cos(x);
    if (c == 0) return 0;
    return s / c;
}
static double f_pow_simple(double b, double e) {
    if (b <= 0) return 0;
    double p = e * f_yl2x(1.0, b);
    int i = (int)p;
    double f = p - i;
    return f_scale(f_2xm1(f) + 1.0, (double)i);
}

static void my_strcpy2(char *d, const char *s) {
    while(*s) *d++ = *s++;
    *d = 0;
}
static void str_cat_s2(char *d, const char *s) {
    int l = strlen(d);
    my_strcpy2(d + l, s);
}

static void my_ftoa(double val, char *buf) {
    if (val < 0) {
        *buf++ = '-';
        val = -val;
    }
    int i = (int)val;
    double f = val - i;
    char temp[32];
    int ti = 0;
    if (i == 0) temp[ti++] = '0';
    else while (i > 0) { temp[ti++] = (i % 10) + '0'; i /= 10; }
    for (int k = 0; k < ti; k++) buf[k] = temp[ti - 1 - k];
    buf += ti;
    *buf++ = '.';
    for (int k = 0; k < 4; k++) {
        f *= 10;
        int d = (int)f;
        *buf++ = d + '0';
        f -= d;
    }
    *buf = 0;
}

/* Parser Global Durumlari */
static const char *p_ptr;
static double var_x = 0.0;

static void skip_space(void) {
    while (*p_ptr == ' ') p_ptr++;
}

static double parse_expr(void);

static double parse_number(void) {
    skip_space();
    double res = 0.0;
    int decimal = 0;
    double div = 1.0;
    /* 'pi' degiskeni */
    if (p_ptr[0] == 'p' && p_ptr[1] == 'i') {
        p_ptr += 2;
        return 3.1415926535;
    }
    /* X degiskeni */
    if (*p_ptr == 'x' || *p_ptr == 'X') {
        p_ptr++;
        return var_x;
    }
    
    while ((*p_ptr >= '0' && *p_ptr <= '9') || *p_ptr == '.') {
        if (*p_ptr == '.') {
            decimal = 1;
        } else {
            if (decimal) {
                div *= 10.0;
                res += (*p_ptr - '0') / div;
            } else {
                res = res * 10.0 + (*p_ptr - '0');
            }
        }
        p_ptr++;
    }
    return res;
}

static double parse_factor(void) {
    skip_space();
    if (*p_ptr == '(') {
        p_ptr++;
        double v = parse_expr();
        if (*p_ptr == ')') p_ptr++;
        return v;
    }
    if (p_ptr[0] == 's' && p_ptr[1] == 'i' && p_ptr[2] == 'n') {
        p_ptr += 3;
        if (*p_ptr == '(') {
            p_ptr++; double v = parse_expr(); if (*p_ptr == ')') p_ptr++;
            return f_sin(v);
        }
    }
    if (p_ptr[0] == 'c' && p_ptr[1] == 'o' && p_ptr[2] == 's') {
        p_ptr += 3;
        if (*p_ptr == '(') {
            p_ptr++; double v = parse_expr(); if (*p_ptr == ')') p_ptr++;
            return f_cos(v);
        }
    }
    if (p_ptr[0] == 't' && p_ptr[1] == 'a' && p_ptr[2] == 'n') {
        p_ptr += 3;
        if (*p_ptr == '(') {
            p_ptr++; double v = parse_expr(); if (*p_ptr == ')') p_ptr++;
            return f_tan(v);
        }
    }
    if (p_ptr[0] == 'l' && p_ptr[1] == 'o' && p_ptr[2] == 'g') {
        p_ptr += 3;
        if (*p_ptr == '(') {
            p_ptr++; double v = parse_expr(); if (*p_ptr == ')') p_ptr++;
            return f_log(v);
        }
    }
    if (p_ptr[0] == 'l' && p_ptr[1] == 'n') {
        p_ptr += 2;
        if (*p_ptr == '(') {
            p_ptr++; double v = parse_expr(); if (*p_ptr == ')') p_ptr++;
            return f_ln(v);
        }
    }
    if (p_ptr[0] == 's' && p_ptr[1] == 'q' && p_ptr[2] == 'r') {
        p_ptr += 3;
        if (*p_ptr == '(') {
            p_ptr++; double v = parse_expr(); if (*p_ptr == ')') p_ptr++;
            return f_sqrt(v);
        }
    }
    return parse_number();
}

static double parse_term(void) {
    double v = parse_factor();
    skip_space();
    while (*p_ptr == '*' || *p_ptr == '/' || *p_ptr == '%' || *p_ptr == '^') {
        char op = *p_ptr++;
        double v2 = parse_factor();
        if (op == '*') v *= v2;
        else if (op == '/') v = (v2 != 0) ? v / v2 : 0;
        else if (op == '%') v = (int)v % (int)v2;
        else if (op == '^') v = f_pow_simple(v, v2);
        skip_space();
    }
    return v;
}

static double parse_expr(void) {
    skip_space();
    double v = parse_term();
    skip_space();
    while (*p_ptr == '+' || *p_ptr == '-') {
        char op = *p_ptr++;
        double v2 = parse_term();
        if (op == '+') v += v2;
        else v -= v2;
        skip_space();
    }
    return v;
}

/* UI VE GRAFIK MOTORU */

#define HIST_SIZE 15
static char history[HIST_SIZE][128];
static int hist_count = 0;

static void add_to_history(const char* text) {
    if (hist_count >= HIST_SIZE) {
        for(int i = 0; i < HIST_SIZE-1; i++) {
            my_strcpy2(history[i], history[i+1]);
        }
        my_strcpy2(history[HIST_SIZE-1], text);
    } else {
        my_strcpy2(history[hist_count++], text);
    }
}

static char plot_canvas[16][45];
static char current_plot_expr[128] = "sin(x)";

static void generate_plot(const char* expr) {
    for(int r = 0; r < 16; r++)
       for(int c = 0; c < 45; c++)
           plot_canvas[r][c] = ' ';

    // X: -10 to 10
    // Y: -5 to 5
    for(int c = 0; c < 45; c++) {
        double x = -10.0 + (20.0 * c / 44.0);
        p_ptr = expr;
        var_x = x;
        double y = parse_expr();
        
        int r = 15 - (int)((y + 5.0) * 15.0 / 10.0);
        if (r >= 0 && r < 16) {
            plot_canvas[r][c] = '*';
        }
        
        // Ciz X ekseni
        if (y >= -0.5 && y <= 0.5 && (r < 0 || r >= 16 || plot_canvas[r][c] == ' ')) {
            int r_axis = 15 - (int)((0 + 5.0) * 15.0 / 10.0);
            if (r_axis >= 0 && r_axis < 16 && plot_canvas[r_axis][c] == ' ')
                plot_canvas[r_axis][c] = '-';
        }
        // Ciz Y ekseni
        if (x >= -0.5 && x <= 0.5) {
            for (int rr = 0; rr < 16; rr++) {
                if (plot_canvas[rr][c] == ' ') plot_canvas[rr][c] = '|';
            }
            if (plot_canvas[r][c] == '-') plot_canvas[r][c] = '+';
        }
    }
}

static char input_buf[128];
static int buf_idx = 0;

static void draw_matlab_screen(void) {
    aegis_theme_t* t = theme_get_current();
    vga_clear(t->bg_color);
    tui_draw_frame("Aegis MATLAB - Bilimsel Hesaplama ve Grafik Merkezi");

    // History Paneli (Sol)
    vga_set_cursor(2, 2);
    vga_puts("[ KOMUT GECMISI ]", t->accent_color);
    for(int i = 0; i < HIST_SIZE; i++) {
        vga_set_cursor(3, 4 + i);
        if (i < hist_count) {
             vga_puts(history[i], t->fg_color);
        }
    }

    // Arayuz Ayirici
    for(int r = 2; r < 20; r++) {
        vga_set_cursor(28, r);
        vga_putc('|', t->border_color);
    }

    // Grafik Paneli (Sag)
    vga_set_cursor(30, 2);
    vga_puts("[ 2D GRAFIK ALANI (y = f(x)) ]", t->accent_color);
    
    vga_set_cursor(30, 3);
    vga_puts("f(x) = ", t->fg_color);
    vga_puts(current_plot_expr, t->accent_color);

    for(int r = 0; r < 16; r++) {
       vga_set_cursor(30, 5 + r);
       for(int c = 0; c < 45; c++) {
          vga_putc(plot_canvas[r][c], (plot_canvas[r][c] == '*') ? t->accent_color : t->border_color);
       }
    }

    // Alt Giris Kutusu
    vga_set_cursor(2, 20);
    for(int i=0; i<76; i++) vga_putc('-', t->border_color);
    vga_set_cursor(2, 21);
    vga_puts(">> ", t->accent_color);
    vga_puts(input_buf, t->fg_color);
    vga_putc('_', t->accent_color);

    tui_draw_bottom_bar("ESC: CIKIS    IPUCU: 'plot sin(x)' veya '5*log(2)' yazarak baslayin");
    vga_swap_buffers();
}

void app_calculator(void) {
    fpu_init();
    my_strcpy2(current_plot_expr, "sin(x)");
    generate_plot(current_plot_expr);
    
    // Clear history on load
    hist_count = 0;
    for (int i = 0; i < 128; i++) input_buf[i] = 0;
    buf_idx = 0;

    draw_matlab_screen();

    while (1) {
        key_event_t ev = input_get();
        if (!ev.pressed) continue;
        int k = ev.key;

        if (k == 27) { /* ESC */
            vga_clear(0x00);
            vga_swap_buffers();
            return;
        }

        if (k == '\n') {
            if (buf_idx > 0) {
                input_buf[buf_idx] = 0;
                
                /* Komutu Isle: plot komutu ise */
                if (input_buf[0] == 'p' && input_buf[1] == 'l' && input_buf[2] == 'o' && input_buf[3] == 't' && input_buf[4] == ' ') {
                    my_strcpy2(current_plot_expr, input_buf + 5);
                    generate_plot(current_plot_expr);
                    add_to_history(input_buf);
                } else {
                    /* Normal matematiksel hesaplama */
                    p_ptr = input_buf;
                    var_x = 0.0;
                    double res = parse_expr();
                    char res_str[32];
                    my_ftoa(res, res_str);
                    
                    char hist_entry[128];
                    my_strcpy2(hist_entry, input_buf);
                    str_cat_s2(hist_entry, " = ");
                    str_cat_s2(hist_entry, res_str);
                    add_to_history(hist_entry);
                }
                
                buf_idx = 0;
                input_buf[0] = 0;
                draw_matlab_screen();
            }
        } 
        else if (k == '\b') {
            if (buf_idx > 0) {
                buf_idx--;
                input_buf[buf_idx] = 0;
                draw_matlab_screen();
            }
        } 
        else if (k >= 32 && k <= 126) {
            if (buf_idx < 125) {
                input_buf[buf_idx++] = (char)k;
                input_buf[buf_idx] = 0;
                draw_matlab_screen();
            }
        }
    }
}
