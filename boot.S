/* ============================================================
   boot.S - AegisOS Multiboot 2 Header (UEFI + Legacy BIOS)
   Desteklenen: x86 (32-bit korumalı mod), 1024x768x32 grafik
   ============================================================ */

/* Multiboot 2 Magic */
#define MB2_MAGIC       0xE85250D6
#define MB2_ARCH        0           /* i386 (32-bit protected mode) */

.section .multiboot2
.align 8

mb2_header_start:
    .long  MB2_MAGIC
    .long  MB2_ARCH
    .long  (mb2_header_end - mb2_header_start)
    .long  -(MB2_MAGIC + MB2_ARCH + (mb2_header_end - mb2_header_start))

/* === Tag 1: Bilgi İstek Etiketi === */
.align 8
tag_info_start:
    .word  1        /* type: information request */
    .word  0        /* flags */
    .long  (tag_info_end - tag_info_start)
    .long  1        /* memory map */
    .long  4        /* basic memory info */
    .long  6        /* memory map (detailed) */
    .long  8        /* framebuffer info */
tag_info_end:

/* === Tag 2: Framebuffer (1024x768x32) === */
.align 8
tag_fb_start:
    .word  5        /* type: framebuffer */
    .word  1        /* flags: optional */
    .long  (tag_fb_end - tag_fb_start)
    .long  1024     /* width */
    .long  768      /* height */
    .long  32       /* depth (bpp) */
tag_fb_end:

/* === Tag 3: Modül Hizalama === */
.align 8
tag_mod_align:
    .word  6        /* type: module alignment */
    .word  0
    .long  8

/* === Son Etiket === */
.align 8
    .word  0        /* type: end tag */
    .word  0
    .long  8
mb2_header_end:

/* ============================================================
   Kernel Giriş Noktası
   ============================================================ */
.section .text
.global _start
.extern kernel_main

_start:
    /* Kesmeleri kapat - çekirdek kendi IDT'sini kurana kadar */
    cli

    /* Stack kur */
    mov $stack_top, %esp
    and $-16, %esp          /* 16-byte hizala (ABI uyumu) */

    /* Multiboot 2 çağrı kuralı: eax = magic, ebx = bilgi yapısı adresi */
    push $0                 /* Stack hizalama dolgusu */
    push $0
    push %ebx               /* Multiboot2 Info Struct adresi */
    push %eax               /* Magic: 0x36d76289 */

    call kernel_main

    /* Kernel dönerse sonsuz döngü */
    cli
.Lhalt:
    hlt
    jmp .Lhalt

/* ============================================================
   Kernel Stack (16 KB)
   ============================================================ */
.section .bss
.align 16
stack_bottom:
    .skip 16384     /* 16 KB */
stack_top:
