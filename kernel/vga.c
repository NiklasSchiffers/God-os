#include "kernel.h"

volatile uint16_t* const VGA = (uint16_t*)0xB8000;
const int VGA_WIDTH  = 80;
const int VGA_HEIGHT = 25;

static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t color = 0x0F;

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA[i] = (uint16_t)' ' | (uint16_t)color << 8;
    cursor_x = 0;
    cursor_y = 0;
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            VGA[cursor_y * VGA_WIDTH + cursor_x] = (uint16_t)' ' | (uint16_t)color << 8;
        }
    } else {
        VGA[cursor_y * VGA_WIDTH + cursor_x] = (uint16_t)c | (uint16_t)color << 8;
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    if (cursor_y >= VGA_HEIGHT) {
        for (int y = 1; y < VGA_HEIGHT; y++)
            for (int x = 0; x < VGA_WIDTH; x++)
                VGA[(y-1)*VGA_WIDTH + x] = VGA[y*VGA_WIDTH + x];
        for (int x = 0; x < VGA_WIDTH; x++)
            VGA[(VGA_HEIGHT-1)*VGA_WIDTH + x] = (uint16_t)' ' | (uint16_t)color << 8;
        cursor_y = VGA_HEIGHT - 1;
    }
}

void vga_print(const char* s) {
    while (*s) vga_putchar(*s++);
}

void vga_print_int(int n) {
    if (n == 0) {
        vga_putchar('0');
        return;
    }
    char buf[16];
    int i = 0;
    if (n < 0) {
        vga_putchar('-');
        n = -n;
    }
    while (n) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    while (i--) vga_putchar(buf[i]);
}
