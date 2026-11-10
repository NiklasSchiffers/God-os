#include "kernel.h"

#define MAX_NAME    32
#define MAX_FILES   32
#define MAX_CONTENT 256

typedef struct {
    char name[MAX_NAME];
    char content[MAX_CONTENT];
    int  is_dir;
    int  parent;
    int  used;
} File;

static File fs[MAX_FILES];
static int  cwd = 0;

void fs_init(void) {
    for (int i = 0; i < MAX_FILES; i++) fs[i].used = 0;

    /* root */
    fs[0].used = 1;
    fs[0].is_dir = 1;
    fs[0].parent = -1;
    kstrcpy(fs[0].name, "/");

    /* demo entries */
    fs[1].used = 1; fs[1].is_dir = 0; fs[1].parent = 0;
    kstrcpy(fs[1].name, "hello.txt");
    kstrcpy(fs[1].content, "Hello from the multi-file kernel!\n");

    fs[2].used = 1; fs[2].is_dir = 1; fs[2].parent = 0;
    kstrcpy(fs[2].name, "docs");

    fs[3].used = 1; fs[3].is_dir = 0; fs[3].parent = 2;
    kstrcpy(fs[3].name, "readme.txt");
    kstrcpy(fs[3].content, "This is a simple in-memory filesystem.\n");

    fs[4].used = 1; fs[4].is_dir = 0; fs[4].parent = 0;
    kstrcpy(fs[4].name, "test.HC");
    kstrcpy(fs[4].content, "// pretend HolyC\n\"Hello from .HC\";\n");
}

static int find_in_dir(int dir, const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fs[i].used) continue;
        if (fs[i].parent != dir) continue;
        if (kstrcmp(fs[i].name, name) == 0) return i;
    }
    return -1;
}

void fs_dir(void) {
    vga_print("Directory of ");
    vga_print(fs_cwd_name());
    vga_print("\n\n");

    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fs[i].used || fs[i].parent != cwd) continue;
        vga_print("  ");
        vga_print(fs[i].name);
        if (fs[i].is_dir) vga_print("/");
        vga_print("\n");
        count++;
    }
    if (count == 0) vga_print("  (empty)\n");
    vga_print("\n");
}

void fs_cd(const char* name) {
    if (!name || name[0] == 0) {
        vga_print("Usage: cd <name>  or  cd /\n");
        return;
    }

    if (name[0] == '/' && name[1] == 0) {
        cwd = 0;
        vga_print("Now in /\n");
        return;
    }

    if (name[0] == '.' && name[1] == '.' && name[2] == 0) {
        if (fs[cwd].parent >= 0) cwd = fs[cwd].parent;
        vga_print("Now in parent\n");
        return;
    }

    int idx = find_in_dir(cwd, name);
    if (idx < 0) {
        vga_print("No such directory\n");
        return;
    }
    if (!fs[idx].is_dir) {
        vga_print("Not a directory\n");
        return;
    }
    cwd = idx;
    vga_print("Now in ");
    vga_print(fs[cwd].name);
    vga_print("\n");
}

void fs_cat(const char* name) {
    if (!name || name[0] == 0) {
        vga_print("Usage: cat <file>\n");
        return;
    }
    int idx = find_in_dir(cwd, name);
    if (idx < 0) {
        vga_print("No such file\n");
        return;
    }
    if (fs[idx].is_dir) {
        vga_print("Is a directory\n");
        return;
    }
    vga_print(fs[idx].content);
}

const char* fs_cwd_name(void) {
    return fs[cwd].name;
}
