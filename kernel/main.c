#include "kernel.h"

void kernel_main(uint32_t magic, multiboot_info_t* info) {
    vga_clear();
    vga_print("SparrowOS\n");

    if (magic != 0x2BADB002) {
        vga_print("ERROR: Not booted by Multiboot!\n");
        for(;;) __asm__ volatile("hlt");
    }

    if (info->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)(uintptr_t)info->mods_addr;
        uint32_t start = mod->mod_start;
        uint32_t end   = mod->mod_end;
        uint32_t size  = end - start;

        vga_print("Module found at 0x");
        vga_print("\n");

        fs_init(start, size);
    } else {
        vga_print("No Multiboot module found!\n");
        vga_print("Filesystem will not work.\n");
    }

    shell_run();

    for(;;) __asm__ volatile("hlt");
}
