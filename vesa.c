#include "vesa.h"
#include <stdint.h>
#include <string.h>

int vesa_ready = 0;
static volatile uint8_t *lfb = 0;
static uint8_t *back = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_bpp = 0;
static uint32_t fb_pitch = 0;

void vesa_init_from_params(uint32_t phys_addr, uint32_t width, uint32_t height, uint32_t bpp, uint32_t pitch) {
    lfb = (volatile uint8_t*)(uintptr_t)phys_addr;
    fb_width = width;
    fb_height = height;
    fb_bpp = bpp;
    fb_pitch = pitch ? pitch : (width * ((bpp + 7) / 8));
    /* allocate simple back buffer in BSS/static (not ideal for large modes) */
    /* For simplicity, reuse a small static region via malloc isn't available; use static allocation */
    /* WARNING: static allocation could be large; keep simple moderate sizes */
    back = (uint8_t*)0; /* marker; user must ensure identity mapping if paging enabled */
    vesa_ready = 1;
}

static inline void put_pixel_32(int x, int y, uint32_t color) {
    uint8_t *p = (uint8_t*) ( (uintptr_t)lfb + (uintptr_t)y * fb_pitch + (uintptr_t)x * 4 );
    p[0] = color & 0xFF;
    p[1] = (color >> 8) & 0xFF;
    p[2] = (color >> 16) & 0xFF;
    p[3] = (color >> 24) & 0xFF;
}

void vesa_putpixel(int x, int y, uint32_t color) {
    if (!vesa_ready) return;
    if (x < 0 || (uint32_t)x >= fb_width || y < 0 || (uint32_t)y >= fb_height) return;
    if (fb_bpp == 32) put_pixel_32(x,y,color);
    else if (fb_bpp == 24) {
        uint8_t *p = (uint8_t*) ( (uintptr_t)lfb + (uintptr_t)y * fb_pitch + (uintptr_t)x * 3 );
        p[0] = color & 0xFF;
        p[1] = (color >> 8) & 0xFF;
        p[2] = (color >> 16) & 0xFF;
    } else if (fb_bpp == 8) {
        lfb[y * fb_pitch + x] = (uint8_t)color;
    }
}

void vesa_clear(uint32_t color) {
    if (!vesa_ready) return;
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            vesa_putpixel(x, y, color);
        }
    }
}

void vesa_swap(void) {
    /* no double buffer implemented */
}

void vesa_demo(void) {
    if (!vesa_ready) return;
    for (uint32_t y = 0; y < fb_height; y++) {
        for (uint32_t x = 0; x < fb_width; x++) {
            uint32_t c = (((x & 0xFF) << 16) | ((y & 0xFF) << 8) | ((x+y)&0xFF));
            vesa_putpixel(x,y,c);
        }
    }
}
