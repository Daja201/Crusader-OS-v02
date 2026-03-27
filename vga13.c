#include "vga13.h"
#include <stddef.h>
#include <string.h>

#define VGA13_FB 0xA0000

static volatile uint8_t *fb = (volatile uint8_t *)VGA13_FB;
static uint8_t backbuf[VGA13_WIDTH * VGA13_HEIGHT];

void vga13_init(void) {
    vga13_clear(0);
    vga13_swap_buffers();
}

void vga13_putpixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= VGA13_WIDTH || y < 0 || y >= VGA13_HEIGHT) return;
    backbuf[y * VGA13_WIDTH + x] = color;
}

uint8_t vga13_getpixel(int x, int y) {
    if (x < 0 || x >= VGA13_WIDTH || y < 0 || y >= VGA13_HEIGHT) return 0;
    return backbuf[y * VGA13_WIDTH + x];
}

void vga13_clear(uint8_t color) {
    memset(backbuf, color, sizeof(backbuf));
}

void vga13_swap_buffers(void) {
    for (size_t i = 0; i < sizeof(backbuf); i++) fb[i] = backbuf[i];
}

void vga13_draw_hline(int x0, int x1, int y, uint8_t color) {
    if (y < 0 || y >= VGA13_HEIGHT) return;
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (x0 < 0) x0 = 0;
    if (x1 >= VGA13_WIDTH) x1 = VGA13_WIDTH - 1;
    for (int x = x0; x <= x1; x++) backbuf[y * VGA13_WIDTH + x] = color;
}

void vga13_draw_vline(int x, int y0, int y1, uint8_t color) {
    if (x < 0 || x >= VGA13_WIDTH) return;
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    if (y0 < 0) y0 = 0;
    if (y1 >= VGA13_HEIGHT) y1 = VGA13_HEIGHT - 1;
    for (int y = y0; y <= y1; y++) backbuf[y * VGA13_WIDTH + x] = color;
}

void vga13_fill_rect(int x, int y, int w, int h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    int x1 = x + w - 1;
    int y1 = y + h - 1;
    if (x1 < 0 || y1 < 0) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 >= VGA13_WIDTH) x1 = VGA13_WIDTH - 1;
    if (y1 >= VGA13_HEIGHT) y1 = VGA13_HEIGHT - 1;
    for (int yy = y; yy <= y1; yy++) {
        uint8_t *row = &backbuf[yy * VGA13_WIDTH];
        for (int xx = x; xx <= x1; xx++) row[xx] = color;
    }
}

void vga13_draw_line(int x0, int y0, int x1, int y1, uint8_t color) {
    int dx = (x1 > x0) ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = -((y1 > y0) ? y1 - y0 : y0 - y1);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (1) {
        vga13_putpixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void vga13_demo(void) {
    for (int y = 0; y < VGA13_HEIGHT; y++)
        for (int x = 0; x < VGA13_WIDTH; x++)
            backbuf[y * VGA13_WIDTH + x] = (uint8_t)((x + y) & 0xFF);
    vga13_draw_line(0, 0, VGA13_WIDTH - 1, VGA13_HEIGHT - 1, 255);
    vga13_draw_line(0, VGA13_HEIGHT - 1, VGA13_WIDTH - 1, 0, 255);
    vga13_swap_buffers();
}
