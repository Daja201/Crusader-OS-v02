#ifndef VGA_H
#define VGA_H
#include "bootinfo.h"
#include "vesa.h"
#include <stdint.h>
void print_char(char c);
void print_string(const char *s);
void print_hex(uint32_t value);
void vga_init();
void vga_backspace();
void vga_center();
void vga_scroll_to_bottom();
extern int boot_has_fb;
#endif