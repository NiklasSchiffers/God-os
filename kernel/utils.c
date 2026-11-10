#include "kernel.h"

int kstrcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

void kstrcpy(char* dest, const char* src) {
    while (*src) *dest++ = *src++;
    *dest = 0;
}

int kstrlen(const char* s) {
    int n = 0;
    while (*s++) n++;
    return n;
}

void kmemset(void* dest, int val, size_t n) {
    uint8_t* p = dest;
    while (n--) *p++ = (uint8_t)val;
}

void* kmemcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    while (n--) *d++ = *s++;
    return dest;
}
