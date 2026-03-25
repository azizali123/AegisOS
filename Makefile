# ============================================================
#  AegisOS v4.0 Makefile
#  Hedef : x86 32-bit, Multiboot 2, UEFI / Legacy BIOS
#  Araç  : i686-elf-gcc (cross) veya gcc -m32
# ============================================================

# Cross-compiler kontrolü
ifeq ($(shell which i686-elf-gcc 2>/dev/null),)
    CC = gcc
    AS = gcc
else
    CC = i686-elf-gcc
    AS = i686-elf-gcc
endif

ifeq ($(shell which i686-elf-ld 2>/dev/null),)
    LD = ld
    LDFLAGS_ARCH = -m elf_i386
else
    LD = i686-elf-ld
    LDFLAGS_ARCH =
endif

# Derleme bayrakları
CFLAGS  = -m32 -ffreestanding -fno-stack-protector -fno-builtin \
          -O2 -Wall -Wextra -Wno-unused-parameter \
          -Ikernel/include -std=gnu11

ASFLAGS = -m32 -ffreestanding

LDFLAGS = $(LDFLAGS_ARCH) -T linker.ld -nostdlib -z noexecstack

# Çıktı dosyaları
KERNEL_BIN = aegis_core.bin
ISO_NAME   = AegisOS_v4.iso

# ============================================================
#  Nesne Dosyaları
# ============================================================
OBJS = \
    kernel/drivers/interrupts.o \
    boot.o \
    kernel/kernel.o \
    kernel/drivers/vga.o \
    kernel/drivers/idt.o \
    kernel/drivers/pit.o \
    kernel/drivers/keyboard.o \
    kernel/drivers/mouse.o \
    kernel/drivers/audio.o \
    kernel/drivers/rtc.o \
    kernel/drivers/pci.o \
    kernel/drivers/usb.o \
    kernel/drivers/network.o \
    kernel/lib/string.o \
    kernel/lib/memory.o \
    kernel/lib/tui.o \
    kernel/fs/filesystem.o \
    kernel/fs/filesystem_compat.o \
    kernel/shell/shell.o \
    kernel/apps/editor.o \
    kernel/apps/calculator.o \
    kernel/apps/explorer.o \
    kernel/apps/clock.o \
    kernel/apps/appmanager.o \
    kernel/apps/system_monitor.o \
    kernel/apps/display_settings.o \
    kernel/apps/firewall_ui.o \
    kernel/system/power.o \
    kernel/system/session.o \
    kernel/system/theme.o \
    kernel/games/snake.o \
    kernel/games/tetris.o \
    kernel/games/pong.o \
    kernel/games/pacman.o \
    kernel/games/xox.o \
    kernel/games/minesweeper.o \
    kernel/games/pentomino.o

# ============================================================
#  Kurallar
# ============================================================
all: $(ISO_NAME)

# Assembly
boot.o: boot.S
	$(AS) $(ASFLAGS) -c $< -o $@

# C dosyaları
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assembly içeren sürücüler
%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

# Kernel bağlama (libgcc 64-bit bolme icin gerekli)
LIBGCC := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
$(KERNEL_BIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBGCC)
	@echo "[OK] Kernel baglandi: $(KERNEL_BIN)"

# ISO oluştur
$(ISO_NAME): $(KERNEL_BIN)
	@mkdir -p iso/boot/grub
	cp $(KERNEL_BIN) iso/boot/
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_NAME) iso --modules="normal multiboot2 all_video video video_fb"
	@echo "[OK] ISO olusturuldu: $(ISO_NAME)"

# UEFI kurulum paketi oluştur (D:\ için)
install: $(KERNEL_BIN)
	@bash install_uefi.sh
	@echo "[OK] D:\\ icin kurulum paketi hazir (install_build/)"

# QEMU test (grafik mod)
run: $(ISO_NAME)
	qemu-system-i386 \
	  -cdrom $(ISO_NAME) \
	  -m 128M \
	  -vga std \
	  -audiodev pa,id=snd0 \
	  -machine pcspk-audiodev=snd0 2>/dev/null || \
	qemu-system-i386 -cdrom $(ISO_NAME) -m 128M -vga std

# QEMU test (UEFI - OVMF gerekir)
run-uefi: $(ISO_NAME)
	qemu-system-x86_64 \
	  -bios /usr/share/ovmf/OVMF.fd \
	  -cdrom $(ISO_NAME) \
	  -m 128M -vga std

# Temizle
clean:
	find . -name "*.o" -not -path "*/Gecmis_Dosyalar/*" -delete
	rm -f $(KERNEL_BIN) $(ISO_NAME)
	@echo "[OK] Temizlendi."

.PHONY: all clean run run-uefi install
