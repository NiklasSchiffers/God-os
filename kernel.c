#define WHITE 0x0F
#define LIGHTCYAN 0x0B
#define LIGHTGREEN 0x0A
#define YELLOW 0x0E
#define RED 0x0C

#define PORT_KEYBOARD_DATA 0x60
#define PORT_KEYBOARD_STATUS 0x64

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define VIDEO_MEM ((volatile char*)0xB8000)
#define TOP_STATUS_LINE_Y   0
#define TOP_STATUS_COLOR    0x1F
#define SHELL_START_Y       1
#define SHELL_MAX_HEIGHT 21
#define HOLYC_WIN_START 21

int system_start_time_minutes = -1;
int cursor_x = 0;
int cursor_y = SHELL_START_Y;
int mouse_x = 40;
int mouse_y = 12;
int mouse_enabled = 0;
char last_vga_char;
unsigned char last_vga_color = WHITE;
char user_input_buffer[512];
unsigned char* jit_memory = (unsigned char*)0x600000;
int shift_pressed = 0;
unsigned char current_text_color = LIGHTGREEN;
unsigned char current_prompt_color = LIGHTCYAN;

char holyc_layout_buffer[4 * 80];
char holyc_data_buffer[4 * 80];

struct Variable {
    char name;
    int value;
    int is_set;
};
struct Variable variable_table[26];

struct Function {
    char name[32];
    const char* body_ptr;
    int is_defined;
};
struct Function function_table[16];

struct multiboot_module {
    unsigned int mod_start;
    unsigned int mod_end;
    unsigned int cmdline;
    unsigned int pad;
};

struct multiboot_info {
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    unsigned int syms;
    unsigned int mmap_length;
    unsigned int mmap_addr;
    unsigned int drives_length;
    unsigned int drives_addr;
    unsigned int config_table;
    unsigned int boot_loader_name;
    unsigned int apm_table;
    unsigned int vbe_control_info;
    unsigned int vbe_mode_info;
    unsigned int vbe_mode;
    unsigned short vbe_interface_seg;
    unsigned short vbe_interface_off;
    unsigned short vbe_interface_len;
    
    unsigned long long framebuffer_addr;
    unsigned int framebuffer_pitch;
    unsigned int framebuffer_width;
    unsigned int framebuffer_height;
    unsigned char framebuffer_bpp;
    unsigned char framebuffer_type;
};

const char* bible_words[25] = {
    "God", "Jesus", "Christ", "Amen", "Faith", "Grace", "Holy", "Spirit", 
    "Heaven", "Earth", "Light", "Love", "Moses", "Creation", "Blessing", "Peace", 
    "Gospel", "Bible", "Glory", "Cross", "Prayer", "Hope", "Truth", "Terry", "Hero"
};
#define BIBLE_WORDS_COUNT 25

void shell_print_string(const char* str, unsigned char color);
void shell_print_char(char c, unsigned char color);
const char* skip_whitespace(const char* str);
const char* parse_factor(const char* ptr, int* val, int* valid);
const char* holyc_run_expression(const char* ptr, int* out_val, int* valid);
void execute_holyc_code(const char* code);
void get_current_time(int* hours, int* minutes, int* seconds);
void print_random_bible_word(unsigned int seed) {
    int index = seed % BIBLE_WORDS_COUNT;
    shell_print_string(bible_words[index], current_text_color);
    shell_print_string(" ", current_text_color);
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

int parse_header_number(const char** src) {
    int res = 0;
    while (**src >= '0' && **src <= '9') {
        res = res * 10 + (**src - '0');
        (*src)++;
    }
    if (**src == '\n' || **src == '\r') (*src)++;
    return res;
}

unsigned char bcd_to_bin(unsigned char bcd) {
    return ((bcd / 16) * 10) + (bcd % 16);
}

unsigned char is_rtc_updating() {
    outb(0x70, 0x0A);
    return (inb(0x71) & 0x80);
}

unsigned char read_rtc_register(unsigned char reg) {
    outb(0x70, reg);
    return inb(0x71);
}

void get_current_time(int* hours, int* minutes, int* seconds) {
    while (is_rtc_updating());
    unsigned char sec  = read_rtc_register(0x00);
    unsigned char min  = read_rtc_register(0x02);
    unsigned char hour = read_rtc_register(0x04);
    unsigned char registerB = read_rtc_register(0x0B);

    if (!(registerB & 0x04)) {
        sec  = bcd_to_bin(sec);
        min  = bcd_to_bin(min);
        hour = bcd_to_bin(hour);
    }
    *hours   = hour;
    *minutes = min;
    *seconds = sec;
}

int get_time_in_minutes() {
    int hours, minutes, seconds;
    get_current_time(&hours, &minutes, &seconds);
    return (hours * 60) + minutes;
}

void draw_top_status_bar(const char* text) {
    int row_offset = TOP_STATUS_LINE_Y * VGA_WIDTH * 2;
    for (int x = 0; x < VGA_WIDTH; x++) {
        VIDEO_MEM[row_offset + (x * 2)] = ' ';
        VIDEO_MEM[row_offset + (x * 2) + 1] = TOP_STATUS_COLOR;
    }
    int i = 0;
    while (text[i] != '\0' && i < 30) { 
        VIDEO_MEM[row_offset + (i * 2)] = text[i];
        i++;
    }
    
    int hours, minutes, seconds;
    get_current_time(&hours, &minutes, &seconds);
    
    int time_start_x = 72; 
    VIDEO_MEM[row_offset + ((time_start_x + 0) * 2)] = '0' + (hours / 10);
    VIDEO_MEM[row_offset + ((time_start_x + 1) * 2)] = '0' + (hours % 10);
    VIDEO_MEM[row_offset + ((time_start_x + 2) * 2)] = ':';
    VIDEO_MEM[row_offset + ((time_start_x + 3) * 2)] = '0' + (minutes / 10);
    VIDEO_MEM[row_offset + ((time_start_x + 4) * 2)] = '0' + (minutes % 10);
    VIDEO_MEM[row_offset + ((time_start_x + 5) * 2)] = ':';
    VIDEO_MEM[row_offset + ((time_start_x + 6) * 2)] = '0' + (seconds / 10);
    VIDEO_MEM[row_offset + ((time_start_x + 7) * 2)] = '0' + (seconds % 10);
}

void JIT_clear() {
    int shell_start_offset = SHELL_START_Y * VGA_WIDTH * 2;
    int shell_end_offset = SHELL_MAX_HEIGHT * VGA_WIDTH * 2;
    for (int i = shell_start_offset; i < shell_end_offset; i += 2) {
        VIDEO_MEM[i] = ' ';
        VIDEO_MEM[i + 1] = current_text_color;
    }
    cursor_x = 0; cursor_y = SHELL_START_Y; 
    last_vga_char = ' '; last_vga_color = current_text_color;
    draw_top_status_bar("--- GOD OS SYSTEM ---");
}

void scroll_screen() {
    for (int y = SHELL_START_Y + 1; y < SHELL_MAX_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            int src_offset = (y * VGA_WIDTH + x) * 2;
            int dest_offset = ((y - 1) * VGA_WIDTH + x) * 2;
            VIDEO_MEM[dest_offset] = VIDEO_MEM[src_offset];
            VIDEO_MEM[dest_offset + 1] = VIDEO_MEM[src_offset + 1];
        }
    }
    int last_row = (SHELL_MAX_HEIGHT - 1) * VGA_WIDTH * 2;
    for (int x = 0; x < VGA_WIDTH; x++) {
        VIDEO_MEM[last_row + x * 2] = ' ';
        VIDEO_MEM[last_row + x * 2 + 1] = current_text_color;
    }
    cursor_y = SHELL_MAX_HEIGHT - 1;
    draw_top_status_bar("--- GOD OS SYSTEM ---");
}
void load_layout_module(const char* hc_code) {
    int max_chars = 4 * VGA_WIDTH;
    for (int i = 0; i < max_chars; i++) holyc_layout_buffer[i] = 0;
    if (hc_code == 0) return;
    const char* ptr = hc_code;
    if (ptr[0] != 'H' || ptr[1] != 'W' || ptr[2] != 'I' || ptr[3] != 'N') return;
    ptr += 4;
    if (*ptr == '\n' || *ptr == '\r') ptr++;
    parse_header_number(&ptr); parse_header_number(&ptr);
    int dest_idx = 0;
    while (*ptr && dest_idx < max_chars) {
        char c = *ptr;
        if (c == '\n') dest_idx = (dest_idx / VGA_WIDTH + 1) * VGA_WIDTH;
        else if (c != '\r') { holyc_layout_buffer[dest_idx] = c; dest_idx++; }
        ptr++;
    }
}

void start_data_buffer_module(const char* hc_code) {
    (void)hc_code;
    for (int i = 0; i < 4 * VGA_WIDTH; i++) holyc_data_buffer[i] = 0;
}

void render_combined_window() {
    int vga_offset = HOLYC_WIN_START * VGA_WIDTH * 2;
    int total_chars = (VGA_HEIGHT - HOLYC_WIN_START) * VGA_WIDTH;
    for (int i = 0; i < total_chars; i++) {
        char layout_c = holyc_layout_buffer[i];
        char data_c = holyc_data_buffer[i];
        char final_c = layout_c;
        if (layout_c == ' ' && data_c != 0) final_c = data_c;
        VIDEO_MEM[vga_offset + i * 2] = final_c;
        VIDEO_MEM[vga_offset + i * 2 + 1] = 0x1E;
    }
}

void draw_mouse(int draw) {
    if (!mouse_enabled) return;
    int offset = (mouse_y * VGA_WIDTH + mouse_x) * 2;
    if (draw) {
        last_vga_char = VIDEO_MEM[offset];
        last_vga_color = VIDEO_MEM[offset + 1];
        VIDEO_MEM[offset] = 'X'; VIDEO_MEM[offset + 1] = RED;
    } else {
        VIDEO_MEM[offset] = last_vga_char; VIDEO_MEM[offset + 1] = last_vga_color;
    }
}

void update_mouse(int dx, int dy) {
    draw_mouse(0);
    mouse_x += dx; mouse_y += dy;
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_x >= VGA_WIDTH) mouse_x = VGA_WIDTH - 1;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_y >= VGA_HEIGHT) mouse_y = VGA_HEIGHT - 1;
    draw_mouse(1);
}

void mouse_paint_pixel() {
    int offset = (mouse_y * VGA_WIDTH + mouse_x) * 2;
    VIDEO_MEM[offset] = 219;
    VIDEO_MEM[offset + 1] = 0x0F;
    last_vga_char = 219;
    last_vga_color = 0x0F;
}

void shell_print_char(char c, unsigned char color) {
    draw_mouse(0);
    if (c == '\n') {
        cursor_x = 0; cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            int offset = (cursor_y * VGA_WIDTH + cursor_x) * 2;
            VIDEO_MEM[offset] = ' '; VIDEO_MEM[offset + 1] = current_text_color;
        }
    } else {
        int offset = (cursor_y * VGA_WIDTH + cursor_x) * 2;
        VIDEO_MEM[offset] = c; VIDEO_MEM[offset + 1] = color;
        cursor_x++;
    }
    if (cursor_x >= VGA_WIDTH) { cursor_x = 0; cursor_y++; }
    if (cursor_y >= SHELL_MAX_HEIGHT) { scroll_screen(); }
    draw_mouse(1);
}

void shell_print_string(const char* str, unsigned char color) {
    while (*str) shell_print_char(*str++, color);
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int atoi(const char* str) {
    int res = 0;
    while (*str >= '0' && *str <= '9') { res = res * 10 + (*str - '0'); str++; }
    return res;
}

void JIT_print_str(const char* str) {
    while (*str) {
        if (*str == '\\' && *(str + 1) == 'n') { shell_print_char('\n', current_text_color); str += 2; continue; }
        shell_print_char(*str++, current_text_color);
    }
}

void print_number(int num) {
    char buf[12]; int i = 10; buf[11] = 0;
    if (num == 0) { shell_print_char('0', current_text_color); return; }
    int is_negative = (num < 0) ? (num = -num, 1) : 0;
    while (num > 0) { buf[i--] = (num % 10) + '0'; num /= 10; }
    if (is_negative) shell_print_char('-', current_text_color);
    shell_print_string(&buf[i + 1], current_text_color);
}

const char* skip_whitespace(const char* str) {
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;
    return str;
}

const char* parse_factor(const char* ptr, int* val, int* valid) {
    ptr = skip_whitespace(ptr);
    if (*ptr >= '0' && *ptr <= '9') {
        *val = atoi(ptr); *valid = 1;
        while (*ptr >= '0' && *ptr <= '9') ptr++;
    } else if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z')) {
        char name = *ptr; ptr++;
        int idx = (name >= 'a') ? (name - 'a') : (name - 'A');
        if (variable_table[idx].is_set) { *val = variable_table[idx].value; *valid = 1; }
        else { *valid = 0; }
    } else if (*ptr == '(') {
        ptr++; ptr = holyc_run_expression(ptr, val, valid);
        ptr = skip_whitespace(ptr);
        if (*ptr == ')') ptr++;
    } else { *valid = 0; }
    return skip_whitespace(ptr);
}

const char* parse_term(const char* ptr, int* val, int* valid) {
    ptr = parse_factor(ptr, val, valid);
    if (!*valid) return ptr;
    while (*ptr == '*' || *ptr == '/') {
        char op = *ptr; ptr++; int next_val = 0;
        ptr = parse_factor(ptr, &next_val, valid);
        if (!*valid) return ptr;
        if (op == '*') *val *= next_val;
        else if (op == '/') { if (next_val != 0) *val /= next_val; else *valid = 0; }
    }
    return ptr;
}

const char* holyc_run_expression(const char* ptr, int* out_val, int* valid) {
    int val = 0; ptr = parse_term(ptr, &val, valid);
    if (!*valid) return ptr;
    while (*ptr == '+' || *ptr == '-') {
        char op = *ptr; ptr++; int next_val = 0;
        ptr = parse_term(ptr, &next_val, valid);
        if (!*valid) return ptr;
        if (op == '+') val += next_val; else val -= next_val;
    }
    *out_val = val;
    return ptr;
}
void emit_u8(unsigned char** stream, unsigned char val) {
    **stream = val; (*stream)++;
}

void emit_i32(unsigned char** stream, int val) {
    *(int*)(*stream) = val; *stream += 4;
}

void compile_holyc_function_to_jit(const char* body_ptr) {
    const char* ptr = skip_whitespace(body_ptr);
    unsigned char* stream = jit_memory;
    
    emit_u8(&stream, 0x55);
    emit_u8(&stream, 0x89); 
    emit_u8(&stream, 0xE5);
    
    while (*ptr && *ptr != '}') {
        ptr = skip_whitespace(ptr);
        if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z')) {
            char v_name = *ptr; const char* check = skip_whitespace(ptr + 1);
            if (*check == '=') {
                ptr = skip_whitespace(check + 1);
                int val = 0; int valid = 0;
                ptr = parse_factor(ptr, &val, &valid);
                if (valid) {
                    emit_u8(&stream, 0xB8); emit_i32(&stream, val);
                    ptr = skip_whitespace(ptr);
                    while (*ptr == '+' || *ptr == '-' || *ptr == '*') {
                        char op = *ptr; ptr = skip_whitespace(ptr + 1);
                        int next_val = 0; ptr = parse_factor(ptr, &next_val, &valid);
                        if (valid) {
                            if (op == '+') { emit_u8(&stream, 0x05); emit_i32(&stream, next_val); } 
                            else if (op == '-') { emit_u8(&stream, 0x2D); emit_i32(&stream, next_val); } 
                            else if (op == '*') { emit_u8(&stream, 0x69); emit_u8(&stream, 0xC0); emit_i32(&stream, next_val); } 
                        }
                    }
                    int idx = (v_name >= 'a') ? (v_name - 'a') : (v_name - 'A');
                    int target_address = (int)&(variable_table[idx].value);
                    emit_u8(&stream, 0xA3); emit_i32(&stream, target_address);
                    variable_table[idx].name = v_name; variable_table[idx].is_set = 1;
                }
            }
        }
        while (*ptr && *ptr != ';' && *ptr != '}') ptr++;
        if (*ptr == ';') ptr++;
    }
    emit_u8(&stream, 0x5D); emit_u8(&stream, 0xC3);
    void (*native_exec)(void) = (void (*)(void))jit_memory;
    native_exec();
    shell_print_string("[JIT] Code successfully compiled and executed on CPU!\n", LIGHTGREEN);
}
void execute_holyc_code(const char* code) {
    const char* ptr = skip_whitespace(code);
    int jit_idx = 0;
    if (strncmp(ptr, "HWIN", 4) == 0 || strncmp(ptr, "HBUF", 4) == 0) return;

    if (strcmp(ptr, "demo_math();") == 0) {
        shell_print_string("Running JIT math script: x = 5 * 10 + 20 - 4;\n", YELLOW);
        execute_holyc_code("U0 Run() { x = 5 * 10 + 20 - 4; }");
        shell_print_string("Result of variable x: ", WHITE); print_number(variable_table['x' - 'a'].value);
        shell_print_char('\n', WHITE); return;
    }
    if (strcmp(ptr, "demo_graphics();") == 0) {
        shell_print_string("Drawing cross lines to framebuffer...\n", YELLOW);
        volatile unsigned short* fb = (volatile unsigned short*)0xFD000000;
        for(int x = 100; x < 200; x++) { fb[200 * 640 + x] = 0xFFFF; fb[x * 640 + 150] = 0xFFFF; }
        shell_print_string("Graphics drawing complete.\n", LIGHTGREEN); return;
    }

    while (*ptr != 0) {
        if (strncmp(ptr, "U0", 2) == 0 || strncmp(ptr, "I32", 3) == 0 || strncmp(ptr, "I64", 3) == 0) {
            while (*ptr && *ptr != ' ' && *ptr != '\t') ptr++;
            ptr = skip_whitespace(ptr);
            char func_name[32]; int fn_idx = 0;
            while (*ptr && *ptr != '(' && *ptr != ' ' && fn_idx < 31) { func_name[fn_idx++] = *ptr++; }
            func_name[fn_idx] = 0; ptr = skip_whitespace(ptr);
            if (*ptr == '(') { while (*ptr && *ptr != ')') ptr++; if (*ptr == ')') ptr++; }
            ptr = skip_whitespace(ptr);
            if (*ptr == '{') {
                ptr++; compile_holyc_function_to_jit(ptr);
                int braces = 1;
                while (*ptr && braces > 0) {
                    if (*ptr == '{') braces++; if (*ptr == '}') braces--; ptr++;
                }
                ptr = skip_whitespace(ptr); continue;
            }
        }
        if (*ptr == '"') {
            static char text_buf[256]; char* dest = text_buf; ptr++;
            while (*ptr && *ptr != '"' && (dest - text_buf) < 254) *dest++ = *ptr++;
            *dest = 0; if (*ptr == '"') ptr++;
            ptr = skip_whitespace(ptr); if (*ptr == ';') ptr = skip_whitespace(ptr + 1);
            jit_memory[jit_idx++] = 0x68; *(int*)&jit_memory[jit_idx] = (int)text_buf; jit_idx += 4;
            jit_memory[jit_idx++] = 0xE8;
            unsigned int relative = (unsigned int)JIT_print_str - ((unsigned int)&jit_memory[jit_idx] + 4);
            *(int*)&jit_memory[jit_idx] = relative; jit_idx += 4;
            jit_memory[jit_idx++] = 0x83; jit_memory[jit_idx++] = 0xC4; jit_memory[jit_idx++] = 0x04;
            jit_memory[jit_idx++] = 0xC3;
            void (*f)(void) = (void (*)(void))jit_memory; f();
            shell_print_char('\n', current_text_color); continue;
        }
        if ((*ptr >= '0' && *ptr <= '9') || *ptr == '(') {
            int out_val = 0, valid = 0; const char* next_ptr = holyc_run_expression(ptr, &out_val, &valid);
            if (valid && next_ptr != ptr) {
                ptr = skip_whitespace(next_ptr); if (*ptr == ';') ptr++;
                print_number(out_val); shell_print_char('\n', current_text_color); continue;
            }
        }
        if (strncmp(ptr, "cls", 3) == 0 || strncmp(ptr, "clear", 5) == 0) { JIT_clear(); return; }
        if (strncmp(ptr, "reboot", 6) == 0) { outb(0x64, 0xFE); return; }
        ptr++;
    }
}

char scancode_to_ascii_us(unsigned char scancode) {
    static const char map_normal[58] = { 0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ' };
    static const char map_shift[58] = { 0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ' };
    if (scancode >= 58) return 0;
    return shift_pressed ? map_shift[scancode] : map_normal[scancode];
}

char get_keypress(void) {
    static unsigned int seed = 0;
    static int last_known_second = -1;
    int h, m, s;
    while (1) {
        seed++;
        get_current_time(&h, &m, &s);
        if (s != last_known_second) {
            last_known_second = s;
            draw_top_status_bar("--- GOD OS SYSTEM ---");
        }
        if (inb(PORT_KEYBOARD_STATUS) & 1) {
            unsigned char scancode = inb(PORT_KEYBOARD_DATA);
            static int ctrl_pressed = 0; 

            if (scancode == 0x1D) { ctrl_pressed = 1; continue; } 
            if (scancode == 0x9D) { ctrl_pressed = 0; continue; } 

            if (scancode == 0x41) { print_random_bible_word(seed); continue; }
            if (scancode == 0x2A || scancode == 0x36) { shift_pressed = 1; continue; }
            if (scancode == 0xAA || scancode == 0xB6) { shift_pressed = 0; continue; }
            if (scancode == 0x3A) { shift_pressed = (shift_pressed == 1) ? 0 : 1; continue; }
            
            if (scancode == 0x48) { update_mouse(0, -1); if (ctrl_pressed) mouse_paint_pixel(); continue; }
            if (scancode == 0x50) { update_mouse(0, 1);  if (ctrl_pressed) mouse_paint_pixel(); continue; }
            if (scancode == 0x4B) { update_mouse(-1, 0); if (ctrl_pressed) mouse_paint_pixel(); continue; }
            if (scancode == 0x4D) { update_mouse(1, 0);  if (ctrl_pressed) mouse_paint_pixel(); continue; }
            
            if (scancode == 0x39 && shift_pressed) { mouse_paint_pixel(); continue; }
            if (scancode & 0x80) continue;
            return scancode_to_ascii_us(scancode);
        }
    }
}


void start_c_based_shell(void) {
    int idx = 0; shell_print_string("Input> ", current_prompt_color);
    while (1) {
        char c = get_keypress();
        if (c == '\n') {
            user_input_buffer[idx] = 0; shell_print_char('\n', current_text_color);
            if (idx > 0) execute_holyc_code(user_input_buffer);
            idx = 0; shell_print_string("Input> ", current_prompt_color);
            render_combined_window();
        } else if (c == '\b') {
            if (idx > 0) { idx--; shell_print_char('\b', current_text_color); }
        } else if (c != 0 && idx < 510) {
            user_input_buffer[idx++] = c; shell_print_char(c, current_text_color);
        }
    }
}

void kernel_main(unsigned int magic, struct multiboot_info* mbi) {
    for (int i = 0; i < 26; i++) variable_table[i].is_set = 0;
    for (int i = 0; i < 16; i++) function_table[i].is_defined = 0;
    JIT_clear();
    shell_print_string("GOD OS \n", YELLOW);
    
    if (magic == 0x2BADB002 && (mbi->flags & (1 << 3)) && mbi->mods_count >= 1) {
        struct multiboot_module* mod0 = (struct multiboot_module*)mbi->mods_addr;
        char* file_module_code = (char*)mod0->mod_start;
        mouse_enabled = 1;
        update_mouse(0, 0);
        if (file_module_code != 0) {
            shell_print_string("Executing boot script...\n", YELLOW);
            execute_holyc_code(file_module_code);
        }
    }
    
    start_c_based_shell();
    while (1);
}

