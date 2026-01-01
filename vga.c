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
    vga_center();
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

void vga_backspace() {
    if (row == 0 && col == 0) return;
    if (col == 0) {
        if (row > 0) {
            row--;
            col = 79;
        } else {
            col = 0;
        }
    } else {
        col--;
    }
    vga[row * 80 + col] = (uint16_t)' ' | (uint16_t)(color << 8);
}

void vga_center() {
    const int W = 80;
    const int H = 25;
    int first_row = 0;
    int delta = first_row - row;
    uint16_t tmp[W * H];
    for (int r = 0; r < H; r++)
        for (int c = 0; c < W; c++)
            tmp[r * W + c] = vga[r * W + c];

    /* clear screen */
    for (int i = 0; i < W * H; i++)
        vga[i] = (uint16_t)' ' | (uint16_t)(color << 8);

    for (int r = 0; r < H; r++) {
        int src_r = r - delta;
        for (int c = 0; c < W; c++) {
            if (src_r >= 0 && src_r < H)
                vga[r * W + c] = tmp[src_r * W + c];
            else
                vga[r * W + c] = (uint16_t)' ' | (uint16_t)(color << 8);
        }
    }

    row = first_row;
    if (col >= W) col = 0;
}
