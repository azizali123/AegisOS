/* ============================================================
   filesystem_compat.c - Uyumluluk Katmanı
   fopen/fclose/fread/fwrite → RAM-FS
   mkdir, fs_change_dir, fs_get_files - Klasör desteği
   ============================================================ */

#include "../include/vga.h"
#include "../include/filesystem.h"
#include "../include/string.h"
#include <stdint.h>

/* ============================================================
   Sahte FILE yapısı
   ============================================================ */
#define MAX_OPEN_FILES 8
#define COMPAT_BUF     FS_MAX_FILESIZE

typedef struct {
    int   used;
    char  name[FS_MAX_FILENAME];
    int   mode_write;
    uint8_t data[COMPAT_BUF];
    uint32_t size;
    uint32_t pos;
} FileHandle;

static FileHandle fh_pool[MAX_OPEN_FILES];

/* ============================================================
   extern: filesystem.c'deki internal veri yapısına erişim
   ============================================================ */
extern int current_dir_id;

/* ============================================================
   fs_change_dir - Dizin değiştir
   "/" veya ".." veya bir klasör adı kabul eder
   ============================================================ */
void fs_change_dir(const char *dir) {
    if (!dir) return;

    /* Kök dizine dön */
    if (strcmp(dir, "/") == 0 || strcmp(dir, "\\") == 0) {
        current_dir_id = -1;
        return;
    }

    /* Üst dizine çık */
    if (strcmp(dir, "..") == 0) {
        if (current_dir_id < 0) return; /* zaten kökteyiz */
        current_dir_id = fs_get_parent_id(current_dir_id);
        return;
    }

    /* İsimle klasör ara (mevcut dizin altında) */
    int idx = fs_find_dir(dir, current_dir_id);
    if (idx >= 0) {
        current_dir_id = idx;
    }
    /* Klasör bulunamazsa, hiçbir şey yapma */
}

/* ============================================================
   readdir - RAM-FS dosyalarını listele
   ============================================================ */
void readdir(const char *path) {
    (void)path;
    char names[FS_MAX_FILES][FS_MAX_FILENAME];
    int n = fs_list(names, FS_MAX_FILES);
    for (int i = 0; i < n; i++) {
        vga_puts("  ", 0x07);
        vga_puts(names[i], 0x0F);
        vga_puts("\n", 0x07);
    }
    if (n == 0) vga_puts("  (bos)\n", 0x08);
}

/* ============================================================
   mkdir - Klasör oluştur (mevcut dizin altında)
   ============================================================ */
void mkdir(const char *path) {
    if (!path || path[0] == 0) return;
    /* Zaten var mı kontrol et */
    int existing = fs_find_dir(path, current_dir_id);
    if (existing >= 0) return; /* zaten var, sorun yok */
    fs_create_in_dir(path, current_dir_id, 1);
}

/* ============================================================
   fs_get_files - Mevcut dizindeki dosya/klasörleri listele
   fs_list_detailed fonksiyonunu kullanır
   ============================================================ */
extern int fs_list_detailed(char names[][FS_MAX_FILENAME], int *is_dir_out, int max_count, int parent_id);

void fs_get_files(int *count, char names[][64], int *is_dir) {
    int n = fs_list_detailed(names, is_dir, 64, current_dir_id);
    if (count) *count = n;
}

/* ============================================================
   fopen
   ============================================================ */
File *fopen(const char *name, const char *mode) {
    FileHandle *fh = 0;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!fh_pool[i].used) { fh = &fh_pool[i]; break; }
    }
    if (!fh) return 0;

    fh->used = 1;
    fh->pos  = 0;
    strncpy(fh->name, name, FS_MAX_FILENAME - 1);
    fh->name[FS_MAX_FILENAME - 1] = 0;

    if (mode[0] == 'r') {
        fh->mode_write = 0;
        int n = fs_read(name, fh->data, COMPAT_BUF - 1);
        if (n < 0) { fh->used = 0; return 0; }
        fh->size = (uint32_t)n;
        fh->data[n] = 0;
    } else {
        fh->mode_write = 1;
        fh->size = 0;
        if (mode[0] == 'a') {
            int n = fs_read(name, fh->data, COMPAT_BUF - 1);
            if (n > 0) { fh->size = (uint32_t)n; fh->pos = fh->size; }
        }
    }
    return (File *)fh;
}

/* ============================================================
   fclose
   ============================================================ */
int fclose(File *f) {
    FileHandle *fh = (FileHandle *)f;
    if (!fh || !fh->used) return -1;
    if (fh->mode_write)
        fs_write(fh->name, fh->data, fh->size);
    fh->used = 0;
    return 0;
}

/* ============================================================
   fread
   ============================================================ */
int fread(void *buf, int size, int count, File *f) {
    FileHandle *fh = (FileHandle *)f;
    if (!fh || !fh->used || fh->mode_write) return 0;
    uint32_t want = (uint32_t)(size * count);
    uint32_t avail = fh->size - fh->pos;
    if (avail < want) want = avail;
    memcpy(buf, fh->data + fh->pos, want);
    fh->pos += want;
    return (int)want;
}

/* ============================================================
   fwrite
   ============================================================ */
int fwrite(const void *buf, int size, int count, File *f) {
    FileHandle *fh = (FileHandle *)f;
    if (!fh || !fh->used || !fh->mode_write) return 0;
    uint32_t want = (uint32_t)(size * count);
    if (fh->pos + want >= COMPAT_BUF) want = COMPAT_BUF - fh->pos - 1;
    memcpy(fh->data + fh->pos, buf, want);
    fh->pos  += want;
    if (fh->pos > fh->size) fh->size = fh->pos;
    return (int)want;
}

/* ============================================================
   fremove
   ============================================================ */
int fremove(const char *name) {
    return fs_delete(name);
}
