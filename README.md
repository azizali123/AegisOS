# AegisOS Core (32-bit Stable)

Bu proje, AegisOS'un kararlı çalışan 32-bit çekirdek sürümüdür.
Eski 64-bit sürümdeki karmaşıklıklardan (Limine/Linker sorunları) arındırılmış, temiz bir temeldir.

## Özellikler
- **Mimari:** 32-bit x86 Protected Mode
- **Bootloader:** GRUB (Multiboot)
- **Görüntü:** VESA Framebuffer (1024x768x32) + VGA Text Fallback
- **Girdi:** PS/2 Klavye Desteği
- **Kabuk:** Komut satırı (ornek: `yardim`, `ls`, `cat`, `touch`, `rm`, `edit`, `explorer`, `tetris`)

## Derleme ve Çalıştırma

Gerekli araçlar: `gcc`, `ld`, `grub-mkrescue`, `xorriso`, `qemu-system-i386`

```bash
# Temizle ve Derle
make clean
make all

# QEMU ile Çalıştır
make run
```

## Yapı
- `kernel/`: Çekirdek kaynak kodu (drivers/, apps/, fs/, shell/, kernel.c)
- `linker.ld`: Bellek düzeni
- `grub.cfg`: Boot menüsü
- `Makefile`: Build sistemi

## Hedefler
Mevcut çekirdekte oyunlar/uygulamalar var; yeni özellikler bu tabana eklenmeye devam edecek.
