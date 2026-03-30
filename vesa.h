#ifndef VESA_H
#define VESA_H
#include <stdint.h>

void vesa_init_from_params(uint32_t phys_addr, uint32_t width, uint32_t height, uint32_t bpp, uint32_t pitch);
void vesa_putpixel(int x, int y, uint32_t color);
void vesa_clear(uint32_t color);
void vesa_swap(void);
void vesa_demo(void);
void vesa_draw_char(char c, int x, int y, uint32_t fg_color, uint32_t bg_color);
void vesa_print_char(char c);
void vesa_print_string(const char *str);
extern int vesa_ready;

#endif
