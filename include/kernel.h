#ifndef KERNEL_H
#define KERNEL_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t string;
    uint32_t reserved;
} multiboot_module_t;

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
} multiboot_info_t;

void vga_clear(void);
void vga_putchar(char c);
void vga_print(const char* s);
void vga_print_int(int n);

char keyboard_getchar(void);

void fs_init(uint32_t iso_start, uint32_t iso_size);
void fs_dir(void);
void fs_cd(const char* name);
void fs_cat(const char* name);
const char* fs_cwd_name(void);
uint32_t fs_read_file(const char* name, char* buffer, uint32_t max_size);

void shell_run(void);

int  kstrcmp(const char* a, const char* b);
void kstrcpy(char* dest, const char* src);
int  kstrlen(const char* s);
void kmemset(void* dest, int val, size_t n);
void* kmemcpy(void* dest, const void* src, size_t n);

#endif
