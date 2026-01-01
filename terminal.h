#ifndef TERMINAL_H
#define TERMINAL_H

#define CMD_BUF_SIZE 128

void terminal_init();
void terminal_key(char c);
void klog(const char* s);
void vga_init();

#endif
