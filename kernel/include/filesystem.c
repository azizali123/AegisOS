#include "../include/filesystem.h"
#include "../include/vga.h"

#define MAX_FILES 10
#define MAX_NAME 16
#define MAX_DATA 256

typedef struct {
  char name[MAX_NAME];
  char data[MAX_DATA];
  int used;
} file_t;

static file_t files[MAX_FILES];

// Yardimci fonksiyon: String kopyalama
static void fs_strcpy(char *dst, const char *src) {
  while (*src)
    *dst++ = *src++;
  *dst = 0;
}

// Yardimci fonksiyon: String karsilastirma
static int fs_streq(const char *a, const char *b) {
  while (*a && *b && *a == *b) {
    a++;
    b++;
  }
  return *a == *b;
}

void fs_init(void) {
  for (int i = 0; i < MAX_FILES; i++) {
    files[i].used = 0;
  }
}

int fs_write_file(const char *name, const char *data) {
  // Once dosya var mi diye bak, varsa uzerine yaz
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].used && fs_streq(files[i].name, name)) {
      fs_strcpy(files[i].data, data);
      return 1; // Guncellendi
    }
  }

  // Yoksa yeni slot bul
  for (int i = 0; i < MAX_FILES; i++) {
    if (!files[i].used) {
      fs_strcpy(files[i].name, name);
      fs_strcpy(files[i].data, data);
      files[i].used = 1;
      return 2; // Yeni olusturuldu
    }
  }
  return 0; // Yer yok
}

char *fs_read_file(const char *name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].used && fs_streq(files[i].name, name)) {
      return files[i].data;
    }
  }
  return 0; // Bulunamadi
}

void fs_delete_file(const char *name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].used && fs_streq(files[i].name, name)) {
      files[i].used = 0;
      vga_puts("  Dosya silindi.\n", 0x0A);
      return;
    }
  }
  vga_puts("  Dosya bulunamadi!\n", 0x0C);
}

void fs_list_files(void) {
  vga_puts("\n=== DOSYALAR ===\n", 0x0E);
  int count = 0;
  for (int i = 0; i < MAX_FILES; i++) {
    if (files[i].used) {
      vga_puts("  - ", 0x07);
      vga_puts(files[i].name, 0x0F);
      vga_puts("\n", 0x07);
      count++;
    }
  }
  if (count == 0) {
    vga_puts("  (Bos)\n", 0x08);
  }
  vga_puts("\n", 0x07);
}