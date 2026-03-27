#ifndef VGA13_H
#define VGA13_H
#include <stdint.h>

#define VGA13_WIDTH 320
#define VGA13_HEIGHT 200

void vga13_init(void);
void vga13_putpixel(int x, int y, uint8_t color);
uint8_t vga13_getpixel(int x, int y);
void vga13_clear(uint8_t color);
void vga13_swap_buffers(void);
void vga13_draw_hline(int x0, int x1, int y, uint8_t color);
void vga13_draw_vline(int x, int y0, int y1, uint8_t color);
void vga13_fill_rect(int x, int y, int w, int h, uint8_t color);
void vga13_draw_line(int x0, int y0, int x1, int y1, uint8_t color);
void vga13_demo(void);

#endif
