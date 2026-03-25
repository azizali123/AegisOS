#include "../include/keyboard.h"
#include "../include/vga.h"
#include "../include/pit.h"
#include <stdint.h>

// UI Constants
#define CW 66
#define CH 22
#define CX 15
#define CY 3

static int get_day_of_week(int d, int m, int y) {
    static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    y += 2000;
    y -= m < 3;
    int day = ( y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    return (day == 0) ? 6 : day - 1; // 0=Pzt, 6=Paz
}

static int days_in_month(int m, int y) {
    if (m == 2) {
        y += 2000;
        int is_leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
        return is_leap ? 29 : 28;
    }
    if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
    return 31;
}

extern void rtc_get_time(uint8_t *day, uint8_t *month, uint8_t *year,
                         uint8_t *hour, uint8_t *min, uint8_t *sec);
extern void rtc_set_time(uint8_t day, uint8_t month, uint8_t year, uint8_t hour,
                         uint8_t min, uint8_t sec);

static int c_day, c_month, c_year, c_hour, c_min, c_sec;
static int cursor_pos = 0;

static void draw_clock_interface(void) {
  vga_clear(0x00);

  // Draw Box
  vga_set_cursor(CX, CY);
  vga_putc('/', 0x0B);
  for (int i = 0; i < CW - 2; i++)
    vga_putc('=', 0x0B);
  vga_putc('\\', 0x0B);

  for (int i = 1; i < CH - 1; i++) {
    vga_set_cursor(CX, CY + i);
    vga_putc('|', 0x0B);
    vga_set_cursor(CX + CW - 1, CY + i);
    vga_putc('|', 0x0B);
  }

  vga_set_cursor(CX, CY + CH - 1);
  vga_putc('\\', 0x0B);
  for (int i = 0; i < CW - 2; i++)
    vga_putc('=', 0x0B);
  vga_putc('/', 0x0B);

  // Title
  vga_set_cursor(CX + 20, CY + 2);
  vga_puts("TARIH, SAAT VE TAKVIM", 0x0E);

  vga_set_cursor(CX + 1, CY + 3);
  vga_puts("----------------------------------------------------------------", 0x0B);

  char buf[3];
  buf[2] = 0;

  // DATE ROW
  vga_set_cursor(CX + 6, CY + 6);
  vga_puts("Tarih: ", 0x0F);

  buf[0] = (c_day / 10) + '0';
  buf[1] = (c_day % 10) + '0';
  vga_puts(buf, cursor_pos == 0 ? 0x70 : 0x0F);
  vga_putc('/', 0x07);

  buf[0] = (c_month / 10) + '0';
  buf[1] = (c_month % 10) + '0';
  vga_puts(buf, cursor_pos == 1 ? 0x70 : 0x0F);
  vga_putc('/', 0x07);

  vga_puts("20", 0x0F);
  buf[0] = (c_year / 10) + '0';
  buf[1] = (c_year % 10) + '0';
  vga_puts(buf, cursor_pos == 2 ? 0x70 : 0x0F);

  // TIME ROW
  vga_set_cursor(CX + 6, CY + 8);
  vga_puts("Saat:  ", 0x0F);

  buf[0] = (c_hour / 10) + '0';
  buf[1] = (c_hour % 10) + '0';
  vga_puts(buf, cursor_pos == 3 ? 0x70 : 0x0F);
  vga_putc(':', 0x07);

  buf[0] = (c_min / 10) + '0';
  buf[1] = (c_min % 10) + '0';
  vga_puts(buf, cursor_pos == 4 ? 0x70 : 0x0F);
  vga_putc(':', 0x07);

  buf[0] = (c_sec / 10) + '0';
  buf[1] = (c_sec % 10) + '0';
  vga_puts(buf, cursor_pos == 5 ? 0x70 : 0x0F);

  // BUTTONS
  vga_set_cursor(CX + 5, CY + 14);
  vga_puts("[ AYARLA ]", cursor_pos == 6 ? 0x70 : 0x0A);

  vga_set_cursor(CX + 22, CY + 14);
  vga_puts("[ CIKIS ]", cursor_pos == 7 ? 0x70 : 0x0C);

  // Instructions
  vga_set_cursor(CX + 4, CY + 18);
  vga_puts("OK TUSLARI ile Secin/Degistirin.", 0x08);

  // ================= TAKVIM / CALENDAR ==================
  vga_set_cursor(CX + 35, CY + 6);
  vga_puts("Pzt Sal Car Prs Cum Cmt Paz", 0x0E);

  int days = days_in_month(c_month, c_year);
  int start_day = get_day_of_week(1, c_month, c_year);

  int cal_y = CY + 8;
  int cal_x = CX + 35;
  for (int d = 1; d <= days; d++) {
      vga_set_cursor(cal_x + start_day * 4, cal_y);
      char dbuf[3];
      dbuf[2] = 0;
      dbuf[0] = (d / 10) + '0';
      dbuf[1] = (d % 10) + '0';
      if (dbuf[0] == '0') dbuf[0] = ' ';

      vga_puts(dbuf, (d == c_day) ? ((cursor_pos == 0) ? 0x4F : 0x70) : 0x0F);

      start_day++;
      if (start_day == 7) {
          start_day = 0;
          cal_y++;
      }
  }

  vga_swap_buffers();
}

void app_clock(void) {
  uint8_t d, m, y, h, min, s;
  rtc_get_time(&d, &m, &y, &h, &min, &s);
  c_day = d;
  c_month = m;
  c_year = y;
  c_hour = h;
  c_min = min;
  c_sec = s;

  /* Input buffer'i temizle */
  while (input_available()) input_get();

  draw_clock_interface();

  uint64_t last_update = pit_get_millis();

  while (1) {
    /* Zaman tabanli guncelleme - her 1000ms (1 saniye) */
    uint64_t now = pit_get_millis();
    if (cursor_pos >= 6 && (now - last_update >= 1000)) {
      last_update = now;
      rtc_get_time(&d, &m, &y, &h, &min, &s);
      c_day = d;
      c_month = m;
      c_year = y;
      c_hour = h;
      c_min = min;
      c_sec = s;
      draw_clock_interface();
    }

    /* Event-tabanli input kontrolu */
    if (!input_available()) {
      /* Kisa bekleme - CPU'yu bosuna yormamak icin */
      for (volatile int i = 0; i < 5000; i++)
        __asm__ volatile("pause");
      vga_swap_buffers();
      continue;
    }

    key_event_t ev = input_get();
    if (!ev.pressed) continue;
    int k = ev.key;

    if (k == 27) {
      vga_clear(0x00);
      vga_swap_buffers();
      return;
    }

    // Navigation
    if (k == KEY_LEFT) {
      cursor_pos--;
      if (cursor_pos < 0) cursor_pos = 7;
    }
    if (k == KEY_RIGHT || k == '\t') {
      cursor_pos++;
      if (cursor_pos > 7) cursor_pos = 0;
    }

    // Value Change (Up/Down)
    if (cursor_pos <= 5) {
      int change = 0;
      if (k == KEY_UP)   change = 1;
      if (k == KEY_DOWN) change = -1;

      if (change != 0) {
        if (cursor_pos == 0) {
          c_day += change;
          if (c_day > 31) c_day = 1;
          if (c_day < 1) c_day = 31;
        }
        if (cursor_pos == 1) {
          c_month += change;
          if (c_month > 12) c_month = 1;
          if (c_month < 1) c_month = 12;
        }
        if (cursor_pos == 2) {
          c_year += change;
          if (c_year > 99) c_year = 0;
          if (c_year < 0) c_year = 99;
        }
        if (cursor_pos == 3) {
          c_hour += change;
          if (c_hour > 23) c_hour = 0;
          if (c_hour < 0) c_hour = 23;
        }
        if (cursor_pos == 4) {
          c_min += change;
          if (c_min > 59) c_min = 0;
          if (c_min < 0) c_min = 59;
        }
        if (cursor_pos == 5) {
          c_sec += change;
          if (c_sec > 59) c_sec = 0;
          if (c_sec < 0) c_sec = 59;
        }
      }
    }

    // Action
    if (k == '\n' || k == ' ') {
      if (cursor_pos == 6) { // AYARLA
        rtc_set_time((uint8_t)c_day, (uint8_t)c_month, (uint8_t)c_year,
                     (uint8_t)c_hour, (uint8_t)c_min, (uint8_t)c_sec);
        vga_set_cursor(CX + 15, CY + 10);
        vga_puts("KAYDEDILDI!", 0x0A);
        vga_swap_buffers();
        pit_sleep_ms(800); /* PIT tabanli kisa bekleme */
      }
      if (cursor_pos == 7) { // CIKIS
        vga_clear(0x00);
        vga_swap_buffers();
        return;
      }
    }

    draw_clock_interface();
  }
}
