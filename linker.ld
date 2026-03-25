/* ============================================================
   linker.ld - AegisOS Linker Script
   Multiboot 2 / UEFI uyumlu, x86 32-bit
   ============================================================ */

ENTRY(_start)

SECTIONS
{
    /* Kernel 1MB adresinden başlar (x86 standart yük adresi) */
    . = 1M;

    /* Multiboot2 header + kod bölümü */
    .text BLOCK(4K) : ALIGN(4K)
    {
        /* Multiboot2 header EN BAŞA yerleştir (ilk 8KB içinde olmak zorunda) */
        *(.multiboot2)
        *(.text)
        *(.text.*)
    }

    /* Salt okunur veri */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* Başlatılmış veri */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
    }

    /* BSS - başlatılmamış/sıfır global veriler */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* Hata ayıklama sembolleri (opsiyonel) */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.GNU-stack)
    }
}
