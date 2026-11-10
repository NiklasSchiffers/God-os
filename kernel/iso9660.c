#include "kernel.h"

#define SECTOR_SIZE 2048

static uint8_t* iso_base = 0;
static uint32_t iso_size = 0;

static uint32_t root_lba = 0;
static uint32_t root_size = 0;

static uint32_t cwd_lba = 0;
static uint32_t cwd_size = 0;
static char cwd_name[64] = "/";

static uint32_t read32(const uint8_t* p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static uint16_t read16(const uint8_t* p) {
    return p[0] | (p[1] << 8);
}

static int iso_init_pvd(void) {
    if (iso_size < 17 * SECTOR_SIZE) return 0;

    uint8_t* pvd = iso_base + 16 * SECTOR_SIZE;

    if (pvd[0] != 1) return 0;                  
    if (pvd[1] != 'C' || pvd[2] != 'D' || pvd[3] != '0' || pvd[4] != '0' || pvd[5] != '1') return 0;

    uint8_t* root = pvd + 156;
    root_lba  = read32(root + 2);               
    root_size = read32(root + 10);              

    cwd_lba  = root_lba;
    cwd_size = root_size;
    kstrcpy(cwd_name, "/");

    return 1;
}

void fs_init(uint32_t start, uint32_t size) {
    iso_base = (uint8_t*)(uintptr_t)start;
    iso_size = size;

    if (!iso_init_pvd()) {
        vga_print("ISO9660: No valid Primary Volume Descriptor found\n");
        iso_base = 0;
    } else {
        vga_print("ISO9660: Mounted successfully\n");
    }
}

static void list_dir(uint32_t lba, uint32_t size) {
    if (!iso_base) {
        vga_print("No ISO mounted\n");
        return;
    }

    uint8_t* dir = iso_base + lba * SECTOR_SIZE;
    uint32_t offset = 0;

    vga_print("Directory of ");
    vga_print(cwd_name);
    vga_print("\n\n");

    while (offset < size) {
        uint8_t* rec = dir + offset;
        uint8_t len = rec[0];
        if (len == 0) break;

        uint8_t name_len = rec[32];
        char name[256];
        kmemcpy(name, rec + 33, name_len);
        name[name_len] = 0;

        for (int i = 0; i < name_len; i++) {
            if (name[i] == ';') {
                name[i] = 0;
                break;
            }
        }

        if (!(name[0] == 0 || (name[0] == 1 && name[1] == 0))) {
            uint8_t flags = rec[25];
            vga_print("  ");
            vga_print(name);
            if (flags & 0x02) vga_print("/");
            vga_print("\n");
        }

        offset += len;
    }
    vga_print("\n");
}

void fs_dir(void) {
    list_dir(cwd_lba, cwd_size);
}

void fs_cd(const char* name) {
    if (!iso_base) {
        vga_print("No ISO mounted\n");
        return;
    }

    if (!name || name[0] == 0) {
        vga_print("Usage: cd <name>  or  cd /\n");
        return;
    }

    if (name[0] == '/' && name[1] == 0) {
        cwd_lba  = root_lba;
        cwd_size = root_size;
        kstrcpy(cwd_name, "/");
        vga_print("Now in /\n");
        return;
    }

    uint8_t* dir = iso_base + cwd_lba * SECTOR_SIZE;
    uint32_t offset = 0;

    while (offset < cwd_size) {
        uint8_t* rec = dir + offset;
        uint8_t len = rec[0];
        if (len == 0) break;

        uint8_t name_len = rec[32];
        char entry_name[256];
        kmemcpy(entry_name, rec + 33, name_len);
        entry_name[name_len] = 0;

        for (int i = 0; i < name_len; i++) {
            if (entry_name[i] == ';') {
                entry_name[i] = 0;
                break;
            }
        }

        if (kstrcmp(entry_name, name) == 0) {
            uint8_t flags = rec[25];
            if (!(flags & 0x02)) {
                vga_print("Not a directory\n");
                return;
            }
            cwd_lba  = read32(rec + 2);
            cwd_size = read32(rec + 10);
            kstrcpy(cwd_name, name);
            vga_print("Now in ");
            vga_print(cwd_name);
            vga_print("\n");
            return;
        }

        offset += len;
    }

    vga_print("No such directory\n");
}

void fs_cat(const char* name) {
    if (!iso_base) {
        vga_print("No ISO mounted\n");
        return;
    }

    if (!name || name[0] == 0) {
        vga_print("Usage: cat <file>\n");
        return;
    }

    uint8_t* dir = iso_base + cwd_lba * SECTOR_SIZE;
    uint32_t offset = 0;

    while (offset < cwd_size) {
        uint8_t* rec = dir + offset;
        uint8_t len = rec[0];
        if (len == 0) break;

        uint8_t name_len = rec[32];
        char entry_name[256];
        kmemcpy(entry_name, rec + 33, name_len);
        entry_name[name_len] = 0;

        for (int i = 0; i < name_len; i++) {
            if (entry_name[i] == ';') {
                entry_name[i] = 0;
                break;
            }
        }

        if (kstrcmp(entry_name, name) == 0) {
            uint8_t flags = rec[25];
            if (flags & 0x02) {
                vga_print("Is a directory\n");
                return;
            }

            uint32_t file_lba  = read32(rec + 2);
            uint32_t file_size = read32(rec + 10);

            uint8_t* data = iso_base + file_lba * SECTOR_SIZE;
            for (uint32_t i = 0; i < file_size; i++) {
                vga_putchar(data[i]);
            }
            if (file_size == 0 || data[file_size-1] != '\n')
                vga_putchar('\n');
            return;
        }

        offset += len;
    }

    vga_print("No such file\n");
}

const char* fs_cwd_name(void) {
    return cwd_name;
}
uint32_t fs_read_file(const char* name, char* buffer, uint32_t max_size) {
    if (!iso_base || !name || !buffer || max_size == 0) return 0;

    uint8_t* dir = iso_base + cwd_lba * SECTOR_SIZE;
    uint32_t offset = 0;

    while (offset < cwd_size) {
        uint8_t* rec = dir + offset;
        uint8_t len = rec[0];
        if (len == 0) break;

        uint8_t name_len = rec[32];
        char entry_name[256];
        kmemcpy(entry_name, rec + 33, name_len);
        entry_name[name_len] = 0;

        for (int i = 0; i < name_len; i++) {
            if (entry_name[i] == ';') {
                entry_name[i] = 0;
                break;
            }
        }

        if (kstrcmp(entry_name, name) == 0) {
            uint8_t flags = rec[25];
            if (flags & 0x02) return 0;

            uint32_t file_lba  = read32(rec + 2);
            uint32_t file_size = read32(rec + 10);

            if (file_size > max_size - 1) file_size = max_size - 1;

            uint8_t* data = iso_base + file_lba * SECTOR_SIZE;
            kmemcpy(buffer, data, file_size);
            buffer[file_size] = 0;
            return file_size;
        }

        offset += len;
    }
    return 0;
}
