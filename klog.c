#include "vga.h"
//#include "vga13.h"
#include "vesa.h"
#include <stdint.h>
#include "klog.h"
#include "string.h"
#include <stdarg.h>

static uint16_t x_letter; // MAX 1600 /8 = 200
static uint16_t y_letter; // MAX 900  /8 = 112.5 = 112  

static void vprintf_internal(const char *fmt, va_list args) {
    char buf[32];
    char ch;
    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] != '%') {
            char out[2] = { fmt[i], 0 };
            klog(out);
            continue;
        }
        i++;
        switch (fmt[i]) {
            case 'd': {
                int val = va_arg(args, int);
                itoa(val, buf, 10);
                kklog(buf);
                break;
            }
            case 'x': {
                uint32_t val = va_arg(args, uint32_t);
                itoa(val, buf, 16);
                kklog(buf);
                break;
            }
            case 's': {
                char *s = va_arg(args, char*);
                if (!s) s = "(null)";
                kklog(s);
                break;
            }
            case 'c': {
                ch = (char)va_arg(args, int);
                char out[2] = { ch, 0 };
                kklog(out);
                break;
            }
            case 'l': {
               if (fmt[i+1] == 'l' && fmt[i+2] == 'u') {
                    unsigned long long v = va_arg(args, unsigned long long);
                    int idx = 0;
                    if (v == 0) {
                        buf[idx++] = '0';
                    } else {
                        unsigned long long div = 1;
                        while (div <= v / 10) div *= 10;
                        while (div > 0) {
                            buf[idx++] = '0' + (v / div);
                            v %= div;
                            div /= 10;
                        }
                    }
                    buf[idx] = '\0';
                    kklog(buf);
                    i += 2; 
                } else {
                    kklog("<?>");
                }
                break;
            }
            case '%':
                kklog("%");
                break;
            default:
                kklog("<?>");
                break;
        }
    }
}

//FUnkce na posun po každém znaku
    //vesa_draw_char('char', X, Y, 0xFFFFFFFF, 0xDB0000);
//FUnkce na posun řádku po X = 200
//Funkce na klog atd (možná přepis jenom vezne ísto serial write char)/
//ostatni klog f-ce


void klog(const char* msg) {
    while (*msg != '\0') {
        char c = *msg;
        if (c == '\n') {
            x_letter = 0;
            y_letter += 8;
        } else {
            vesa_draw_char(c, x_letter, y_letter, 0x00FF00, 0x000000);
            x_letter += 8;
            if (x_letter >= 600) {
                x_letter = 0;
                y_letter += 8;
            }
        }

        if (y_letter >= 900) {
            y_letter = 0;
        }

        msg++;
    }
}

void kklog(const char* msg) {
    klog(msg); 
    klog("\n");
}
void klogf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf_internal(fmt, args);
    va_end(args);
    //y_letter += 8;
    
}

void kklogf(const char *fmt, ...) {
    klogf(fmt);
       
}

/*
void klog(const char* msg) {
    x_letter = x_letter + 8;
    if (x_letter > 200)
        x_letter = 0;
        y_letter = y_letter + 1;
    serial_write_char(msg);
}
void klog(const char* msg) {
    if (vesa_ready) {
        vesa_print_string(msg);
        vesa_print_char('\n');
    } else {
        print_string(msg);
        print_char('\n');
    }
    serial_write(msg);
    serial_write("\n");
}
// Minimal serial (COM1) output for early boot logging
static inline void serial_outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t serial_inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static void serial_write(const char *str) {
    while (*str) {
        serial_write_char(*str++);
    }
}
void kklog(const char* msg) {
    if (vesa_ready) {
        vesa_print_string(msg);
    } else {
        print_string(msg);
    }
    serial_write(msg);
}
void klog_hex(uint32_t val) {
    char buf[32];
    itoa(val, buf, 16);
    kklog("0x");
    kklog(buf); 
}
void kklogf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf_internal(fmt, args);
    va_end(args);
}
void klogf(const char *fmt, ...) {
}
*/