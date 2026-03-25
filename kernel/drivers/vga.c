#include "../include/vga.h"
#include "../include/font.h"
#include <stdint.h>
#include <stddef.h>

#define MB2_TAG_FRAMEBUFFER 8

typedef struct { uint32_t type; uint32_t size; } MB2_Tag;
typedef struct {
    uint32_t type; uint32_t size;
    uint64_t fb_addr; uint32_t fb_pitch;
    uint32_t fb_width; uint32_t fb_height;
    uint8_t fb_bpp; uint8_t fb_type; uint16_t reserved;
} __attribute__((packed)) MB2_Tag_FB;

static uint8_t  *framebuffer = NULL;
static uint32_t  fb_pitch = 0, fb_width = 0, fb_height = 0, fb_bpp = 0;
static int       text_mode = 0;
static volatile uint16_t *vga_text = (volatile uint16_t *)0xB8000;
#define TEXT_COLS 80
#define TEXT_ROWS 25
static int term_x = 0, term_y = 0;
#define F_W 8
#define F_H 16

/* Double Buffer (BSS'te barınacak, 1024x768x4 byte = 3MB max) */
#define MAX_FB_SIZE (1024 * 768 * 4)
__attribute__((aligned(16))) static uint8_t backbuffer[MAX_FB_SIZE];

/* Dirty Rect / Değişen alan takibi */
static int dirty_min_x = 999999, dirty_min_y = 999999;
static int dirty_max_x = -1, dirty_max_y = -1;

static inline void mark_dirty(int x, int y, int w, int h) {
    if (x < dirty_min_x) dirty_min_x = x;
    if (y < dirty_min_y) dirty_min_y = y;
    if (x + w > dirty_max_x) dirty_max_x = x + w;
    if (y + h > dirty_max_y) dirty_max_y = y + h;
}

static const uint32_t palette[16] = {
    0x000000,0x0000AA,0x00AA00,0x00AAAA,
    0xAA0000,0xAA00AA,0xAA5500,0xAAAAAA,
    0x555555,0x5555FF,0x55FF55,0x55FFFF,
    0xFF5555,0xFF55FF,0xFFFF55,0xFFFFFF
};
static inline void vga_outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}

static void text_update_cursor(void){
    uint16_t pos=(uint16_t)(term_y*TEXT_COLS+term_x);
    vga_outb(0x3D4,0x0F);vga_outb(0x3D5,(uint8_t)(pos&0xFF));
    vga_outb(0x3D4,0x0E);vga_outb(0x3D5,(uint8_t)(pos>>8));
}
static void text_scroll(void){
    for(int i=0;i<TEXT_COLS*(TEXT_ROWS-1);i++) vga_text[i]=vga_text[i+TEXT_COLS];
    for(int i=0;i<TEXT_COLS;i++) vga_text[(TEXT_ROWS-1)*TEXT_COLS+i]=0x0720;
    term_y=TEXT_ROWS-1;
}
static void text_putc(char c,uint8_t color){
    if(c=='\n'){term_x=0;term_y++;}
    else if(c=='\r'){term_x=0;}
    else if(c=='\b'){if(term_x>0)term_x--;}
    else if(c=='\t'){term_x=(term_x+8)&~7;}
    else{vga_text[term_y*TEXT_COLS+term_x]=((uint16_t)color<<8)|(uint8_t)c;term_x++;}
    if(term_x>=TEXT_COLS){term_x=0;term_y++;}
    if(term_y>=TEXT_ROWS)text_scroll();
    text_update_cursor();
}

void vga_draw_pixel(int x,int y,uint32_t color){
    if(!framebuffer||(uint32_t)x>=fb_width||(uint32_t)y>=fb_height)return;
    uint32_t off=(uint32_t)y*fb_pitch+(uint32_t)x*(fb_bpp/8);
    if(off >= MAX_FB_SIZE) return;
    
    if(fb_bpp==32)*((uint32_t*)(backbuffer+off))=color;
    else if(fb_bpp==24){backbuffer[off]=(uint8_t)(color&0xFF);backbuffer[off+1]=(uint8_t)((color>>8)&0xFF);backbuffer[off+2]=(uint8_t)((color>>16)&0xFF);}
    mark_dirty(x, y, 1, 1);
}

static void gfx_draw_char(int cx,int cy,char ch,uint32_t fg,uint32_t bg){
    unsigned char c = (unsigned char)ch;
    int index = 0;
    if(c >= 32 && c <= 126) index = c - 32;
    else if(c >= 128 && c <= 139) index = 95 + (c - 127); /* 128 => 96 */
    else index = '?' - 32; /* invalid chars fallback to ? */
    
    const uint8_t *g=font8x16[index];
    for(int r=0;r<F_H;r++){uint8_t bits=g[r];for(int c=0;c<F_W;c++)vga_draw_pixel(cx+c,cy+r,(bits&(0x80>>c))?fg:bg);}
    mark_dirty(cx, cy, F_W, F_H);
}
static int gfx_cols(void){return(int)(fb_width/F_W);}
static int gfx_rows(void){return(int)(fb_height/F_H);}

static void gfx_scroll(void){
    if(!framebuffer)return;
    uint32_t rb=(uint32_t)F_H*fb_pitch;
    uint32_t tot=(uint32_t)(gfx_rows()-1)*rb;
    for(uint32_t i=0;i<tot;i++)backbuffer[i]=backbuffer[i+rb];
    for(uint32_t i=0;i<rb;i++)backbuffer[tot+i]=0;
    term_y=gfx_rows()-1;
    mark_dirty(0, 0, fb_width, fb_height);
}

static void gfx_putc(char c,uint8_t color){
    uint32_t fg=palette[color&0x0F],bg=palette[(color>>4)&0x0F];
    if(c=='\n'){term_x=0;term_y++;}
    else if(c=='\r'){term_x=0;}
    else if(c=='\b'){if(term_x>0){term_x--;gfx_draw_char(term_x*F_W,term_y*F_H,' ',fg,bg);}}
    else if(c=='\t'){term_x=(term_x+8)&~7;}
    else{gfx_draw_char(term_x*F_W,term_y*F_H,c,fg,bg);term_x++;}
    if(term_x>=gfx_cols()){term_x=0;term_y++;}
    if(term_y>=gfx_rows())gfx_scroll();
}

void vga_draw_rect_fill(int x,int y,int w,int h,uint32_t color){
    for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)vga_draw_pixel(i,j,color);
    mark_dirty(x, y, w, h);
}
void vga_draw_rect(int x,int y,int w,int h,uint32_t color){
    for(int i=x;i<x+w;i++){vga_draw_pixel(i,y,color);vga_draw_pixel(i,y+h-1,color);}
    for(int j=y;j<y+h;j++){vga_draw_pixel(x,j,color);vga_draw_pixel(x+w-1,j,color);}
    mark_dirty(x, y, w, h);
}
void vga_draw_line(int x0,int y0,int x1,int y1,uint32_t color){
    int minx = x0 < x1 ? x0 : x1;
    int miny = y0 < y1 ? y0 : y1;
    int maxx = x0 > x1 ? x0 : x1;
    int maxy = y0 > y1 ? y0 : y1;
    int dx=x1-x0,dy=y1-y0,sx=(dx>0)?1:-1,sy=(dy>0)?1:-1;
    dx=dx<0?-dx:dx;dy=dy<0?-dy:dy;int err=dx-dy;
    while(1){vga_draw_pixel(x0,y0,color);if(x0==x1&&y0==y1)break;int e2=2*err;if(e2>-dy){err-=dy;x0+=sx;}if(e2<dx){err+=dx;y0+=sy;}}
    mark_dirty(minx, miny, maxx - minx + 1, maxy - miny + 1);
}

void vga_putc(char c,uint8_t color){if(text_mode)text_putc(c,color);else gfx_putc(c,color);}
void vga_puts(const char *s,uint8_t color){while(*s)vga_putc(*s++,color);}
void vga_put_hex(uint32_t val,uint8_t color){
    char buf[11];buf[0]='0';buf[1]='x';
    for(int i=9;i>=2;i--){int n=val&0xF;buf[i]=(char)(n<10?'0'+n:'A'+n-10);val>>=4;}
    buf[10]=0;vga_puts(buf,color);
}
void vga_put_dec(uint32_t val,uint8_t color){
    char buf[12];int i=10;buf[11]=0;
    if(val==0){vga_putc('0',color);return;}
    buf[i]=0;while(val>0){buf[--i]=(char)('0'+val%10);val/=10;}
    vga_puts(buf+i,color);
}
void vga_put_int(int val,uint8_t color){if(val<0){vga_putc('-',color);vga_put_dec((uint32_t)(-val),color);}else vga_put_dec((uint32_t)val,color);}
void vga_newline(void){vga_putc('\n',0x07);}
void vga_clear(uint8_t color){
    if(text_mode){uint16_t e=((uint16_t)color<<8)|' ';for(int i=0;i<TEXT_COLS*TEXT_ROWS;i++)vga_text[i]=e;term_x=0;term_y=0;text_update_cursor();}
    else{uint32_t bg=palette[(color>>4)&0x0F];vga_draw_rect_fill(0,0,(int)fb_width,(int)fb_height,bg);term_x=0;term_y=0;}
}
void vga_set_cursor(int x,int y){term_x=x;term_y=y;}
void vga_get_cursor(int *x,int *y){*x=term_x;*y=term_y;}
uint32_t vga_get_width(void){return fb_width?fb_width:TEXT_COLS*F_W;}
uint32_t vga_get_height(void){return fb_height?fb_height:TEXT_ROWS*F_H;}
void vga_scroll(void){if(!text_mode)gfx_scroll();else text_scroll();}

/* Double Buffer'i gercek ekrana basan fonksiyon */
void vga_swap_buffers(void) {
    if (text_mode || !framebuffer) return;
    if (dirty_max_x < 0) return; /* Değisen bir yer yok */
    
    /* Sinirlari guvenceye al */
    if (dirty_min_x < 0) dirty_min_x = 0;
    if (dirty_min_y < 0) dirty_min_y = 0;
    if (dirty_max_x >= (int)fb_width) dirty_max_x = fb_width - 1;
    if (dirty_max_y >= (int)fb_height) dirty_max_y = fb_height - 1;

    for (int y = dirty_min_y; y <= dirty_max_y; y++) {
        uint32_t row_offset = y * fb_pitch;
        uint32_t start_col = dirty_min_x * (fb_bpp / 8);
        uint32_t end_col = (dirty_max_x + 1) * (fb_bpp / 8);
        
        uint32_t start_offset = row_offset + start_col;
        uint32_t bytes_to_copy = end_col - start_col;
        
        if (start_offset + bytes_to_copy <= MAX_FB_SIZE) {
            /* Hafıza kopyalama - stdlib olmadığı için kendimiz kopyalıyoruz */
            uint8_t *dst = framebuffer + start_offset;
            uint8_t *src = backbuffer + start_offset;
            for(uint32_t i=0; i<bytes_to_copy; i++) {
                dst[i] = src[i];
            }
        }
    }
    
    /* Rectangle'i sifirla */
    dirty_min_x = 999999; dirty_min_y = 999999;
    dirty_max_x = -1; dirty_max_y = -1;
}

void vga_init_graphics(void *mb2_info_ptr){
    /* En başta temizle */
    for(int i=0; i<MAX_FB_SIZE; i++) backbuffer[i] = 0;

    if(!mb2_info_ptr){text_mode=1;return;}
    uint8_t *p=(uint8_t*)mb2_info_ptr+8;
    uint32_t total=*((uint32_t*)mb2_info_ptr);
    uint8_t *end=(uint8_t*)mb2_info_ptr+total;
    while(p<end){
        MB2_Tag *tag=(MB2_Tag*)p;
        if(tag->type==0)break;
        if(tag->type==MB2_TAG_FRAMEBUFFER){
            MB2_Tag_FB *fb=(MB2_Tag_FB*)tag;
            if(fb->fb_type==1&&fb->fb_bpp>=24){
                framebuffer=(uint8_t*)(uintptr_t)(uint32_t)fb->fb_addr;
                fb_pitch=fb->fb_pitch;fb_width=fb->fb_width;
                fb_height=fb->fb_height;fb_bpp=fb->fb_bpp;
                text_mode=0;vga_clear(0x00);
                vga_swap_buffers();
                return;
            }
        }
        p+=(tag->size+7)&~7u;
    }
    text_mode=1;
}
