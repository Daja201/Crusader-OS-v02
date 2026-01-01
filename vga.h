#ifndef VGA_H
#define VGA_H

#include <stdint.h>

void print_char(char c);
void print_string(const char *s);
void print_hex(uint32_t value);

void vga_init();

#endif
