/* ============================================================
   idt.c - IDT (Kesme Tanımlayıcı Tablosu)
   - 256 giriş, 32-bit korumalı mod
   - PIC yeniden eşleme (IRQ0-7 → INT32-39, IRQ8-15 → INT40-47)
   - IRQ0 (PIT) ve IRQ1 (Klavye) özel işleyicilere bağlı
   ============================================================ */
#include "../include/idt.h"
#include "../include/io.h"
#include "../include/vga.h"
#include "../include/pit.h"
#include "../include/keyboard.h"

struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

extern void isr_generic(void);
extern void isr_irq0(void);
extern void isr_irq1(void);

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_lo = (uint16_t)(base & 0xFFFF);
    idt[num].base_hi = (uint16_t)((base >> 16) & 0xFFFF);
    idt[num].sel     = sel;
    idt[num].always0 = 0;
    idt[num].flags   = flags;
}

/* C-tabanlı kesme işleyicileri */
void c_irq0_handler(void) {
    pit_irq_handler();
    /* EOI idt.S'de gönderiliyor */
}

void c_irq1_handler(void) {
    keyboard_poll();
    outb(0x20, 0x20);  /* PIC Master EOI */
}

void c_generic_handler(void) {
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}

void idt_init(void) {
    idtp.limit = (uint16_t)(sizeof(struct idt_entry) * 256 - 1);
    idtp.base  = (uint32_t)&idt;

    /* Tüm girişleri sıfırla */
    for (int i = 0; i < 256; i++)
        idt_set_gate((uint8_t)i, 0, 0, 0);

    /* PIC yeniden eşleme:
       Master → INT 0x20-0x27 (32-39)
       Slave  → INT 0x28-0x2F (40-47) */
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);

    /* Tüm IRQ'ları maskele, sonra seçili açık */
    outb(0x21, 0xFF); outb(0xA1, 0xFF);

    /* Mevcut CS değerini oku */
    uint16_t cs_sel;
    __asm__ volatile("mov %%cs, %0" : "=r"(cs_sel));

    /* Tüm girdileri genel işleyiciye yönlendir */
    for (int i = 0; i < 256; i++)
        idt_set_gate((uint8_t)i, (uint32_t)isr_generic, cs_sel, 0x8E);

    /* IRQ0 (PIT) ve IRQ1 (Klavye) özel işleyicileri */
    idt_set_gate(0x20, (uint32_t)isr_irq0, cs_sel, 0x8E);
    idt_set_gate(0x21, (uint32_t)isr_irq1, cs_sel, 0x8E);

    /* IDT'yi yükle */
    __asm__ volatile("lidt %0" : : "m"(idtp));

    /* IRQ0 (PIT) ve IRQ1 (Klavye) maskesini aç */
    outb(0x21, 0xFC);   /* 0xFC = sadece IRQ0 ve IRQ1 açık */
    outb(0xA1, 0xFF);   /* Slave PIC tümü kapalı */

    /* Kesmeleri etkinleştir */
    __asm__ volatile("sti");

    vga_puts("  [IDT] Kesme Mimarisi Yuklendi (PIT+KBD aktif)\n", 0x0A);
}
