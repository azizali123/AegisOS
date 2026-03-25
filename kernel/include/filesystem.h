#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>

#define FS_MAX_FILENAME  64
#define FS_MAX_FILES    128
#define FS_MAX_FILESIZE (64 * 1024)

/* Her dosya/klasör için yapı */
typedef struct {
    char     name[FS_MAX_FILENAME];
    uint32_t size;
    uint8_t  data[FS_MAX_FILESIZE];
    int      used;
    int      is_dir;        /* 1 = klasör, 0 = dosya */
    int      parent_id;     /* Üst dizin indeksi, -1 = kök dizin */
} FsFile;

/* Installer uyumluluğu için */
typedef struct {
    uint32_t start_lba;
    uint32_t total_sectors;
    uint8_t  type;
} PartitionEntry;

#define KEY_ENTER '\n'
#define KEY_ESC   27

/* Temel dosya sistemi API */
void     fs_init(void);
int      fs_create(const char *name);
int      fs_create_in_dir(const char *name, int parent_id, int is_dir);
int      fs_write(const char *name, const uint8_t *data, uint32_t size);
int      fs_read(const char *name, uint8_t *buf, uint32_t max_size);
int      fs_delete(const char *name);
int      fs_exists(const char *name);
int      fs_list(char names[][FS_MAX_FILENAME], int max_count);
uint32_t fs_get_size(const char *name);
int      ata_detect(void);
int      fs_find_dir(const char *name, int parent_id);
int      fs_list_detailed(char names[][FS_MAX_FILENAME], int *is_dir_out, int max_count, int parent_id);
int      fs_get_parent_id(int idx);

/* Uyumluluk Katmanı */
typedef struct FileOpaque File;
File*  fopen(const char *name, const char *mode);
int    fclose(File *f);
int    fread(void *buf, int size, int count, File *f);
int    fwrite(const void *buf, int size, int count, File *f);
int    fremove(const char *name);
void   readdir(const char *path);
void   fs_change_dir(const char *dir);

/* Dizin fonksiyonları */
void fs_get_files(int *count, char names[][64], int *is_dir);
void mkdir(const char *path);
extern int current_dir_id;  /* Mevcut dizin: -1 = kök, >=0 = dosya indeksi */

/* Installer stub'ları */
int  fs_get_fat16_partitions(PartitionEntry *parts, int max_count);
void fs_format_partition(int idx);

#endif /* FILESYSTEM_H */
