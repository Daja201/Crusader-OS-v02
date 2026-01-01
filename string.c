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