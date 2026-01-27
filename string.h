#ifndef STRING_H
#define STRING_H
#include <stddef.h>
int strcmp(const char* a, const char* b);
size_t strlen(const char* s);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
char* itoa(int value, char* buffer, int base);
#endif
