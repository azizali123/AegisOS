/* ============================================================
   rtc.c - CMOS/RTC Saat Sürücüsü
   BCD ve Binary mod otomatik algılama
   ============================================================ */
#include "../include/rtc.h"
#include "../include/vga.h"
#include <stdint.h>

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

static inline void cmos_outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t cmos_inb(uint16_t p){uint8_t r;__asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p));return r;}

static uint8_t get_rtc_reg(int reg) {
    cmos_outb(CMOS_ADDR, (uint8_t)(0x80 | reg));
    uint8_t val = cmos_inb(CMOS_DATA);
    cmos_outb(CMOS_ADDR, 0x00);
    return val;
}
static void set_rtc_reg(int reg, uint8_t val) {
    cmos_outb(CMOS_ADDR, (uint8_t)(0x80 | reg));
    cmos_outb(CMOS_DATA, val);
    cmos_outb(CMOS_ADDR, 0x00);
}
static uint8_t bcd2bin(uint8_t b){return (uint8_t)(((b>>4)*10)+(b&0x0F));}
static uint8_t bin2bcd(uint8_t b){return (uint8_t)(((b/10)<<4)|(b%10));}

static void wait_rtc_ready(void) {
    int t = 100000;
    while ((get_rtc_reg(0x0A) & 0x80) && t--);
}

void rtc_get_time(uint8_t *day, uint8_t *month, uint8_t *year,
                  uint8_t *hour, uint8_t *min, uint8_t *sec) {
    wait_rtc_ready();
    uint8_t s=get_rtc_reg(0x00), m=get_rtc_reg(0x02), h=get_rtc_reg(0x04);
    uint8_t d=get_rtc_reg(0x07), mo=get_rtc_reg(0x08), y=get_rtc_reg(0x09);
    uint8_t rb=get_rtc_reg(0x0B);
    if(!(rb & 0x04)){s=bcd2bin(s);m=bcd2bin(m);h=bcd2bin(h);d=bcd2bin(d);mo=bcd2bin(mo);y=bcd2bin(y);}
    /* 12 saatlik mod kontrolü */
    if(!(rb & 0x02) && (h & 0x80)){h=(uint8_t)(((h&0x7F)%12)+12);}
    *sec=s; *min=m; *hour=h; *day=d; *month=mo; *year=y;
}

void rtc_set_time(uint8_t day,uint8_t month,uint8_t year,
                  uint8_t hour,uint8_t min,uint8_t sec) {
    if (day < 1 || day > 31 || month < 1 || month > 12 || hour > 23 || min > 59 || sec > 59) return;
    __asm__ volatile("cli");
    wait_rtc_ready();
    uint8_t rb=get_rtc_reg(0x0B);
    set_rtc_reg(0x0B, rb|0x80);
    set_rtc_reg(0x00,bin2bcd(sec));  set_rtc_reg(0x02,bin2bcd(min));
    set_rtc_reg(0x04,bin2bcd(hour)); set_rtc_reg(0x07,bin2bcd(day));
    set_rtc_reg(0x08,bin2bcd(month));set_rtc_reg(0x09,bin2bcd(year));
    set_rtc_reg(0x0B, rb & ~0x80);
    __asm__ volatile("sti");
}

void rtc_print_time(void) {
    uint8_t sec,min,hour,day,month,year;
    rtc_get_time(&day,&month,&year,&hour,&min,&sec);
    char buf[3]; buf[2]=0;
    vga_puts("\n  Tarih: ", 0x0B);
    buf[0]=(char)('0'+day/10);   buf[1]=(char)('0'+day%10);   vga_puts(buf,0x0F);
    vga_putc('/',0x07);
    buf[0]=(char)('0'+month/10); buf[1]=(char)('0'+month%10); vga_puts(buf,0x0F);
    vga_puts("/20",0x07);
    buf[0]=(char)('0'+year/10);  buf[1]=(char)('0'+year%10);  vga_puts(buf,0x0F);
    vga_puts("\n  Saat:  ", 0x0B);
    buf[0]=(char)('0'+hour/10);  buf[1]=(char)('0'+hour%10);  vga_puts(buf,0x0F);
    vga_putc(':',0x07);
    buf[0]=(char)('0'+min/10);   buf[1]=(char)('0'+min%10);   vga_puts(buf,0x0F);
    vga_putc(':',0x07);
    buf[0]=(char)('0'+sec/10);   buf[1]=(char)('0'+sec%10);   vga_puts(buf,0x0F);
    vga_puts("\n",0x07);
}
