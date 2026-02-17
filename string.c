//MY OWN STRCMP and STRLEN UWU
#include "string.h"

int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

//Just casualy listening to LINkin PARk while writing some C code :3

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char*)dest;
    const unsigned char *s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char*)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

char* itoa(int value, char* buffer, int base) {
    if (base < 2 || base > 16) {
        buffer[0] = 0;
        return buffer;
    }

    char *ptr = buffer;
    char *ptr1 = buffer;
    char tmp_char;
    int tmp_value;

    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = 0;
        return buffer;
    }

    int sign = 0;
    if (value < 0 && base == 10) {
        sign = 1;
        value = -value;
    }

    while (value != 0) {
        tmp_value = value % base;
        *ptr++ = (tmp_value < 10)
            ? tmp_value + '0'
            : tmp_value - 10 + 'A';
        value /= base;
    }

    if (sign)
        *ptr++ = '-';

    *ptr-- = 0;

    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return buffer;
}

char* strncpy(char* dest, const char* src, int n) {
    int i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}