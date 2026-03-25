#include "../include/vga.h"
#include "../include/keyboard.h"
#include "../include/filesystem.h"

// Wait for a little bit to simulate time passing
static void sleep_sm() {
    for (volatile int i = 0; i < 50000000; i++) {
        __asm__ volatile("nop");
    }
}

static void draw_setup_frame() {
    vga_clear(0x1F); // White text on Blue background
    vga_set_cursor(0, 0);
    // Draw top bar
    vga_puts(" Aegis OS Kurulum Programi ", 0x70); // Black text on Gray background
    vga_puts("                                                                 \n", 0x70);
    vga_set_cursor(0, 2);
}

static void draw_bottom_bar(const char *text) {
    vga_set_cursor(0, 24);
    vga_puts(" ", 0x70);
    vga_puts(text, 0x70);
    
    // Fill the rest
    int len = 0;
    while(text[len]) len++;
    for (int i = len + 1; i < 80; i++) {
        vga_putc(' ', 0x70);
    }
}

void itoa(int num, char* str) {
    int i = 0;
    if (num == 0) { str[i++] = '0'; str[i] = '\0'; return; }
    while (num != 0) {
        int rem = num % 10;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / 10;
    }
    str[i] = '\0';
    int start = 0; int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void run_installer() {
    draw_setup_frame();
    vga_set_cursor(2, 4);
    vga_puts("Aegis OS Kurulumuna Hosgeldiniz.", 0x1F);
    
    vga_set_cursor(2, 6);
    vga_puts("Bu program Aegis OS'i bilgisayariniza kuracaktir.", 0x1F);
    
    vga_set_cursor(2, 8);
    vga_puts("Secenekleriniz:", 0x1F);
    vga_set_cursor(4, 10);
    vga_puts("ENTER  -  Sistemi Kur (Sadece FAT16 Diskleri Bulur)", 0x1F);
    vga_set_cursor(4, 11);
    vga_puts("R      -  Sistemi Kurmadan Baslat (Live CD Mode)", 0x1F);
    vga_set_cursor(4, 12);
    vga_puts("F3     -  Cikis yap", 0x1F);
    
    draw_bottom_bar("ENTER=Kur  R=Canli Calistir  F3=Cikis");

    while (1) {
        int key = keyboard_read_key();
        
        if (key == KEY_ENTER) {
            draw_setup_frame();
            vga_set_cursor(2, 4);
            vga_puts("Sistemdeki diskler ve bolumler taraniliyor...", 0x1F);
            sleep_sm();
            
            PartitionEntry parts[4];
            int count = fs_get_fat16_partitions(parts, 4);
            
            if (count == 0) {
                vga_set_cursor(2, 6);
                vga_puts("Sistemde FAT16 formatlanmis bir bolum bulunamadi!", 0x4F);
                vga_set_cursor(2, 7);
                vga_puts("Lutfen ana diskinizde bir FAT16 bolumu olusturun.", 0x1F);
                draw_bottom_bar("Ana menuye donmek icin herhangi bir tusa basin.");
                keyboard_read_key();
                run_installer();
                return;
            }
            
            draw_setup_frame();
            vga_set_cursor(2, 4);
            vga_puts("Kurulum yapilacak FAT16 bolumu secin (Sadece bu bolum wipe edilecek):", 0x1F);
            
            for (int i = 0; i < count; i++) {
                vga_set_cursor(4, 6 + i);
                vga_putc('1' + i, 0x1F);
                vga_puts(") FAT16 Bolum - Boyut: ", 0x1F);
                uint32_t mb = parts[i].total_sectors / 2048; // 512 bytes per sector. 2048 sectors = 1MB
                char buf[16];
                itoa(mb, buf);
                vga_puts(buf, 0x1F);
                vga_puts(" MB  (LBA: ", 0x1F);
                itoa(parts[i].start_lba, buf);
                vga_puts(buf, 0x1F);
                vga_puts(")", 0x1F);
            }
            
            draw_bottom_bar("Secim yapmak icin 1-4 tuslarina basin. ESC = Iptal");
            
            int selected = -1;
            while (1) {
                int k = keyboard_read_key();
                if (k == KEY_ESC) {
                    run_installer();
                    return;
                }
                if (k >= '1' && k < '1' + count) {
                    selected = k - '1';
                    break;
                }
            }
            
            draw_setup_frame();
            vga_set_cursor(2, 4);
            vga_puts("Kurulum baslatiliyor. Ana disk yapisi KORUNACAK.", 0x1E);
            
            sleep_sm();
            vga_set_cursor(2, 6);
            vga_puts("Asama 1: Sadece secili FAT16 bolumu temizleniyor...", 0x1F);
            
            fs_format_partition(selected);
            
            sleep_sm();
            vga_set_cursor(2, 8);
            vga_puts("Asama 2: Aegis OS dosyalari kopyalaniyor...", 0x1F);
            
            fs_init();
            
            sleep_sm();
            vga_set_cursor(2, 11);
            vga_puts("Kurulum tamamlandi!", 0x1E);
            
            draw_bottom_bar("ENTER=Bilgisayari Yeniden Baslat");
            while(keyboard_read_key() != KEY_ENTER);
            
            __asm__ volatile("movb $0xFE, %al; outb %al, $0x64");
            while(1);
        }
        else if (key == 'r' || key == 'R') {
            vga_clear(0x07);
            break;
        }
        else if (key == KEY_F3) {
            draw_setup_frame();
            vga_set_cursor(2, 4);
            vga_puts("Kurulum iptal edildi. Bilgisayar kapatiliyor...", 0x1F);
            sleep_sm();
            while(1) { __asm__ volatile("hlt"); }
        }
    }
}
