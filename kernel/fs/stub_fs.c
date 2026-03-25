#include "../include/filesystem.h"
#include "../include/vga.h"

// Dummy File System Implementation

void fs_init(void) {
  // vga_puts("FS: Initialized (Dummy)\n", 0x07);
}

int fs_write_file(const char *name, const char *content) {
  (void)name;
  (void)content; // Unused
  vga_puts("FS: Write not implemented\n", 0x0C);
  return 0; // Success? Or fail? Let's say fail (0) or success (1). usually 0 is
            // success in C.
            // wait, usually 0 is success. let's return 0.
}

char *fs_read_file(const char *name) {
  (void)name;
  vga_puts("FS: Read not implemented\n", 0x0C);
  return 0; // NULL
}

int fs_delete_file(const char *name) {
  (void)name;
  vga_puts("FS: Delete not implemented\n", 0x0C);
  return 0;
}

void fs_list_files(void) { vga_puts("FS: No files (Dummy FS)\n", 0x07); }
