#include "kernel.h"

#define CMD_BUF 128
#define SCRIPT_BUF 4096

static char cmd_buf[CMD_BUF];
static int  cmd_len = 0;

static void process_command(const char* line);   // Vorwärtsdeklaration

static void run_script(const char* filename) {
    static char script[SCRIPT_BUF];

    uint32_t size = fs_read_file(filename, script, SCRIPT_BUF);
    if (size == 0) {
        vga_print("Cannot read script: ");
        vga_print(filename);
        vga_print("\n");
        return;
    }

    vga_print("Running script: ");
    vga_print(filename);
    vga_print("\n");

    char* p = script;
    while (*p) {
        // Eine Zeile extrahieren
        char line[CMD_BUF];
        int i = 0;

        while (*p && *p != '\n' && *p != '\r' && i < CMD_BUF - 1) {
            line[i++] = *p++;
        }
        line[i] = 0;

        // Newline überspringen
        while (*p == '\n' || *p == '\r') p++;

        // Leere Zeilen und Kommentare überspringen
        if (line[0] == 0 || line[0] == '#') continue;

        vga_print("> ");
        vga_print(line);
        vga_print("\n");

        process_command(line);
    }
}

static void process_command(const char* full) {
    // Kopie machen, weil wir sie verändern
    char buf[CMD_BUF];
    kstrcpy(buf, full);

    char* p = buf;
    while (*p == ' ') p++;
    if (*p == 0) return;

    char* cmd = p;
    while (*p && *p != ' ') p++;
    char saved = *p;
    *p = 0;
    char* arg = (saved) ? p + 1 : p;
    while (*arg == ' ') arg++;

    if (kstrcmp(cmd, "dir") == 0 || kstrcmp(cmd, "ls") == 0) {
        fs_dir();
    } else if (kstrcmp(cmd, "cd") == 0) {
        fs_cd(arg);
    } else if (kstrcmp(cmd, "cat") == 0) {
        fs_cat(arg);
    } else if (kstrcmp(cmd, "clear") == 0) {
        vga_clear();
    } else if (kstrcmp(cmd, "run") == 0) {
        if (arg[0] == 0) {
            vga_print("Usage: run <script.sh>\n");
        } else {
            run_script(arg);
        }
    } else if (kstrcmp(cmd, "help") == 0) {
        vga_print("Commands:\n");
        vga_print("  dir / ls     - list files\n");
        vga_print("  cd <name>    - change directory\n");
        vga_print("  cd /         - go to root\n");
        vga_print("  cat <file>   - show file\n");
        vga_print("  run <file>   - run a script (.sh)\n");
        vga_print("  clear        - clear screen\n");
        vga_print("  help         - this help\n");
    } else {
        vga_print("Unknown command. Type 'help'\n");
    }
}

void shell_run(void) {
    vga_print("Shell ready. Type 'help'\n\n");

    while (1) {
        vga_print(fs_cwd_name());
        vga_print("> ");

        cmd_len = 0;
        while (1) {
            char c = keyboard_getchar();

            if (c == '\n') {
                vga_putchar('\n');
                cmd_buf[cmd_len] = 0;
                process_command(cmd_buf);
                break;
            } else if (c == '\b') {
                if (cmd_len > 0) {
                    cmd_len--;
                    vga_putchar('\b');
                }
            } else if (cmd_len < CMD_BUF - 1) {
                cmd_buf[cmd_len++] = c;
                vga_putchar(c);
            }
        }
    }
}
