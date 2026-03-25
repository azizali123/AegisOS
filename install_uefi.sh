#!/bin/bash
# ============================================================
#  AegisOS Kurulum Scripti (UEFI Sistemler Icin)
#  Bu script GRUB EFI bootloader olusturur
# ============================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/install_build"

echo ""
echo "  =================================================="
echo "  |     AEGIS OS v4.0 - UEFI KURULUM HAZIRLAYICISI |"
echo "  =================================================="
echo ""

# Temizle ve olustur
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR/EFI/BOOT"
mkdir -p "$BUILD_DIR/boot/grub"

# Kernel binary kopyala
if [ ! -f "$SCRIPT_DIR/aegis_core.bin" ]; then
    echo "[HATA] aegis_core.bin bulunamadi! Once 'make' ile derleyin."
    exit 1
fi

cp "$SCRIPT_DIR/aegis_core.bin" "$BUILD_DIR/boot/aegis_core.bin"
echo "[OK] Kernel kopyalandi: boot/aegis_core.bin"

# GRUB config kopyala
cp "$SCRIPT_DIR/grub_install.cfg" "$BUILD_DIR/boot/grub/grub.cfg"
echo "[OK] GRUB yapilandirmasi kopyalandi: boot/grub/grub.cfg"

# GRUB EFI standalone binary olustur
echo "[..] GRUB EFI bootloader olusturuluyor..."

grub-mkstandalone \
    --format=x86_64-efi \
    --output="$BUILD_DIR/EFI/BOOT/BOOTX64.EFI" \
    --locales="" \
    --fonts="" \
    --themes="" \
    --install-modules="normal multiboot multiboot2 boot all_video video video_fb video_bochs video_cirrus part_gpt part_msdos fat ext2 configfile echo test search search_fs_uuid search_label ls cat" \
    "boot/grub/grub.cfg=$BUILD_DIR/boot/grub/grub.cfg" \
    "boot/aegis_core.bin=$BUILD_DIR/boot/aegis_core.bin"

echo "[OK] GRUB EFI olusturuldu: EFI/BOOT/BOOTX64.EFI"

# i386 (32-bit) GRUB EFI de olustur (bazi UEFI 32-bit kullanir)
if [ -d "/usr/lib/grub/i386-efi" ]; then
    grub-mkstandalone \
        --format=i386-efi \
        --output="$BUILD_DIR/EFI/BOOT/BOOTIA32.EFI" \
        --locales="" \
        --fonts="" \
        --themes="" \
        "boot/grub/grub.cfg=$BUILD_DIR/boot/grub/grub.cfg" \
        "boot/aegis_core.bin=$BUILD_DIR/boot/aegis_core.bin"
    echo "[OK] 32-bit GRUB EFI de olusturuldu: EFI/BOOT/BOOTIA32.EFI"
fi

echo ""
echo "  =================================================="
echo "  |          KURULUM DOSYALARI HAZIR!               |"
echo "  =================================================="
echo ""
echo "  Olusturulan dosyalar: $BUILD_DIR/"
echo ""
echo "  Dizin yapisi:"
find "$BUILD_DIR" -type f | sort | while read f; do
    size=$(stat -c%s "$f" 2>/dev/null || echo "?")
    echo "    $(echo "$f" | sed "s|$BUILD_DIR/||")  ($size bytes)"
done
echo ""
echo "  =================================================="
echo "  |              KURULUM ADIMLARI                   |"
echo "  =================================================="
echo ""
echo "  1. Windows'ta AegisOS bolumunu bulun (2GB FAT)"
echo "     (Disk Yonetimi'nde surucu harfini not edin: ornek F:)"
echo ""
echo "  2. PowerShell'i YONETICI OLARAK acin ve su komutu calistirin:"
echo "     (F: yerine sizin surucu harfinizi yazin)"
echo ""
echo '     Copy-Item -Path "install_build\*" -Destination "F:\" -Recurse -Force'
echo ""
echo "  3. UEFI boot menusune ekleme (PowerShell Yonetici):"
echo '     bcdedit /copy {bootmgr} /d "Aegis OS"'
echo "     (Cikan GUID'i not edin, ornek: {xxxxxxxx-xxxx-...})"
echo '     bcdedit /set {GUID} path \EFI\BOOT\BOOTX64.EFI'
echo '     bcdedit /set {GUID} device partition=F:'
echo ""
echo "  4. Yeniden baslatip BIOS/UEFI'de boot menusunden"
echo '     "Aegis OS" secenegini secin.'
echo ""
echo "  ALTERNATIF: BIOS/UEFI Boot Menu (F12/F2/DEL) ile"
echo "  dogrudan FAT bolumdeki EFI dosyasindan boot edin."
echo ""
