#include "kernel.h"

static uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static int shift_pressed = 0;
static int caps_lock = 0;

static const char scancode_normal[] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
    'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' '
};

static const char scancode_shift[] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"','~', 0, '|',
    'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' '
};

char keyboard_getchar(void) {
    while (1) {
        while (!(inb(0x64) & 1));
        uint8_t sc = inb(0x60);

        if (sc & 0x80) {
            sc &= 0x7F;
            if (sc == 0x2A || sc == 0x36)
                shift_pressed = 0;
            continue;
        }

        if (sc == 0x2A || sc == 0x36) {
            shift_pressed = 1;
            continue;
        }
        if (sc == 0x3A) {
            caps_lock = !caps_lock;
            continue;
        }

        char c = 0;
        if (sc < sizeof(scancode_normal)) {
            c = shift_pressed ? scancode_shift[sc] : scancode_normal[sc];

            if (caps_lock) {
                if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
                else if (c >= 'A' && c <= 'Z' && !shift_pressed) c = c - 'A' + 'a';
            }
        }

        if (c) return c;
    }
}
