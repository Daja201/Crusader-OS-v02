#include "vga.h"


#define VGA_ADDR 0xB8000
static uint16_t *vga = (uint16_t*)VGA_ADDR;
static uint8_t row = 0;
static uint8_t col = 0;
static uint8_t color = 0x0F;




//vga gphcs

void vga_init() {
    for (int i = 0; i < 80*25; i++)
        vga[i] = (uint16_t)' ' | (uint16_t)(color << 8);
}





void print_char(char c) {
    if (c == '\n') {
        row++;
        col = 0;
        return;
    }

    vga[row * 80 + col] = (uint16_t)c | (uint16_t)(color << 8);

    col++;
    if (col >= 80) {
        col = 0;
        row++;
    }
}


void print_string(const char *s) {
    while (*s) print_char(*s++);
}

void print_hex(uint32_t value) {
    char hex[] = "0123456789ABCDEF";
    print_string("0x");

    for (int i = 28; i >= 0; i -= 4)
        print_char(hex[(value >> i) & 0xF]);
}

