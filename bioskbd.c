#include "bioskbd.h"
#include "vga.h"

static inline unsigned char inb(unsigned short port)
{
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a" (ret) : "Nd" (port));
    return ret;
}

static unsigned char read_scancode(void)
{
    while (!(inb(0x64) & 1));
    return inb(0x60);
}

/* partial set-1 map */
static const char map[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z',
    'x','c','v','b','n','m',',','.','/',0, '*',0,' '};

char bios_getchar_echo(void)
{
    unsigned char sc;
    char c = 0;
    for (;;) {
        sc = read_scancode();
        if (sc & 0x80) continue; /* ignore break codes */
        if (sc < sizeof(map)) c = map[sc];
        if (c == 0) continue;
        if (c == '\b') {
            vga_backspace();
            continue; /* don't return on backspace */
        }
        /* echo and return */
        print_char(c);
        return c;
    }
}
