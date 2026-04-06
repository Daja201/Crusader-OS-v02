#ifndef STRING_H
#define STRING_H
#include <stddef.h>
#include <stdint.h>
int strcmp(const char* a, const char* b);
size_t strlen(const char* s);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
char* itoa(int value, char* buffer, int base);
int atoi(const char* s);
long strtol(const char* nptr, char** endptr, int base);
int strcasecmp(const char* a, const char* b);
char* strncpy(char* dest, const char* src, size_t n);
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void insw(uint16_t port, void* addr, int words) {
    asm volatile("rep insw" : "+D"(addr), "+c"(words) : "d"(port) : "memory");
}

static inline void outsw(uint16_t port, const void* addr, int words) {
    asm volatile("rep outsw" : "+S"(addr), "+c"(words) : "d"(port));
}

#define tolower(c) ((c) >= 'A' && (c) <= 'Z' ? (c) + 32 : (c))
#endif
