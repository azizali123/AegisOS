/* ============================================================
   filesystem.c - AegisOS RAM Dosya Sistemi
   Klasör desteği eklenmiş versiyon
   ============================================================ */
#include "../include/filesystem.h"
#include "../include/io.h"
#include "../include/string.h"
#include "../include/vga.h"
#include <stdint.h>

/* ---- ATA port sabitleri ---- */
#define ATA_STATUS  0x1F7
#define ATA_DRIVE   0x1F6
#define ATA_SR_BSY  0x80

/* ---- RAM-FS ---- */
static FsFile files[FS_MAX_FILES];
static int    fs_ready = 0;

/* Mevcut dizin indeksi: -1 = kök dizin */
int current_dir_id = -1;

/* ---- ATA Güvenli Tespit ---- */
int ata_detect(void) {
    uint8_t st = inb(ATA_STATUS);
    if (st == 0xFF || st == 0x00) {
        vga_puts("[FS] ATA/IDE yok - RAM-FS aktif.\n", 0x0E);
        return 0;
    }
    outb(ATA_DRIVE, 0xA0);
    io_wait(); io_wait(); io_wait(); io_wait();
    st = inb(ATA_STATUS);
    if (st == 0xFF) { vga_puts("[FS] ATA cihaz yok - RAM-FS aktif.\n", 0x0E); return 0; }
    uint32_t t = 50000;
    while ((inb(ATA_STATUS) & ATA_SR_BSY) && t-- > 0) io_wait();
    if (t == 0) { vga_puts("[FS] ATA BSY zaman asimi - RAM-FS aktif.\n", 0x0E); return 0; }
    vga_puts("[FS] ATA surucusu tespit edildi.\n", 0x0A);
    return 1;
}

/* ---- Public API ---- */

void fs_init(void) {
    memset(files, 0, sizeof(files));
    /* Tüm parent_id'leri -1 yap (kök) */
    for (int i = 0; i < FS_MAX_FILES; i++) {
        files[i].parent_id = -1;
        files[i].is_dir = 0;
        files[i].used = 0;
    }
    fs_ready = 1;
    current_dir_id = -1;
    ata_detect();

    /* Varsayılan hoşgeldin dosyası (kök dizinde) */
    const char *msg = "AegisOS v4.0 RAM Dosya Sistemi\nHosgeldiniz!\n";
    fs_create("readme.txt");
    fs_write("readme.txt", (const uint8_t *)msg, (uint32_t)strlen(msg));

    vga_puts("[FS] RAM-FS hazir (", 0x0A);
    vga_put_dec(FS_MAX_FILES, 0x0F);
    vga_puts(" slot, ", 0x07);
    vga_put_dec(FS_MAX_FILESIZE / 1024, 0x0F);
    vga_puts(" KB/dosya).\n", 0x07);
}

/* Belirli parent altında isimle ara */
static FsFile *find_slot(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (files[i].used && strcmp(files[i].name, name) == 0
            && files[i].parent_id == current_dir_id)
            return &files[i];
    return 0;
}

/* Herhangi bir parent altında isimle ara (indeks döndürür) */
static int find_slot_idx(const char *name, int parent_id) {
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (files[i].used && strcmp(files[i].name, name) == 0
            && files[i].parent_id == parent_id)
            return i;
    return -1;
}

/* Public: İsimle dizin ara, indeks döndür */
int fs_find_dir(const char *name, int parent_id) {
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (files[i].used && files[i].is_dir
            && strcmp(files[i].name, name) == 0
            && files[i].parent_id == parent_id)
            return i;
    return -1;
}

/* Mevcut dizinde dosya oluştur */
int fs_create(const char *name) {
    /* Zaten var mı? */
    if (find_slot_idx(name, current_dir_id) >= 0) return 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].used) {
            strncpy(files[i].name, name, FS_MAX_FILENAME - 1);
            files[i].name[FS_MAX_FILENAME-1] = 0;
            files[i].size = 0;
            files[i].used = 1;
            files[i].is_dir = 0;
            files[i].parent_id = current_dir_id;
            return 1;
        }
    }
    return -1;
}

/* Belirli dizinde dosya/klasör oluştur */
int fs_create_in_dir(const char *name, int parent_id, int is_dir) {
    /* Zaten var mı? */
    if (find_slot_idx(name, parent_id) >= 0) return 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].used) {
            strncpy(files[i].name, name, FS_MAX_FILENAME - 1);
            files[i].name[FS_MAX_FILENAME-1] = 0;
            files[i].size = 0;
            files[i].used = 1;
            files[i].is_dir = is_dir;
            files[i].parent_id = parent_id;
            return i; /* oluşturulan indeks */
        }
    }
    return -1;
}

int fs_write(const char *name, const uint8_t *data, uint32_t size) {
    FsFile *f = find_slot(name);
    if (!f) { if (fs_create(name) < 0) return -1; f = find_slot(name); }
    if (!f) return -1;
    if (size > FS_MAX_FILESIZE) size = FS_MAX_FILESIZE;
    memcpy(f->data, data, size);
    f->size = size;
    return (int)size;
}

int fs_read(const char *name, uint8_t *buf, uint32_t max_size) {
    FsFile *f = find_slot(name);
    if (!f) return -1;
    uint32_t n = f->size < max_size ? f->size : max_size;
    memcpy(buf, f->data, n);
    return (int)n;
}

int fs_delete(const char *name) {
    FsFile *f = find_slot(name);
    if (!f) return -1;
    f->used = 0; f->size = 0; f->name[0] = 0;
    return 0;
}

int fs_exists(const char *name) { return find_slot(name) ? 1 : 0; }

uint32_t fs_get_size(const char *name) {
    FsFile *f = find_slot(name);
    return f ? f->size : 0;
}

/* Mevcut dizindeki dosyaları listele */
int fs_list(char names[][FS_MAX_FILENAME], int max_count) {
    int n = 0;
    for (int i = 0; i < FS_MAX_FILES && n < max_count; i++)
        if (files[i].used && files[i].parent_id == current_dir_id)
            strncpy(names[n++], files[i].name, FS_MAX_FILENAME);
    return n;
}

/* Detaylı dosya listeleme: isim + is_dir bilgisi */
int fs_list_detailed(char names[][FS_MAX_FILENAME], int *is_dir_out, int max_count, int parent_id) {
    int n = 0;
    for (int i = 0; i < FS_MAX_FILES && n < max_count; i++) {
        if (files[i].used && files[i].parent_id == parent_id) {
            strncpy(names[n], files[i].name, FS_MAX_FILENAME);
            if (is_dir_out) is_dir_out[n] = files[i].is_dir;
            n++;
        }
    }
    return n;
}

/* Belirli bir indeksin parent_id'sini döndür */
int fs_get_parent_id(int idx) {
    if (idx < 0 || idx >= FS_MAX_FILES) return -1;
    if (!files[idx].used) return -1;
    return files[idx].parent_id;
}

/* Installer stub'ları */
int fs_get_fat16_partitions(PartitionEntry *parts, int max_count) {
    (void)parts; (void)max_count;
    return 0;
}

void fs_format_partition(int idx) {
    (void)idx;
}
