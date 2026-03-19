#include "vga.h"
#include <stdint.h>
#include "klog.h"
#include "string.h"
#include <stdarg.h>

// Minimal serial (COM1) output for early boot logging
static inline void serial_outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t serial_inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void serial_write_char(char c) {
    const uint16_t port = 0x3F8; // COM1
    static int inited = 0;
    if (!inited) {
        serial_outb(port + 1, 0x00); // Disable all interrupts
        serial_outb(port + 3, 0x80); // Enable DLAB (set baud rate divisor)
        serial_outb(port + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
        serial_outb(port + 1, 0x00); //                  (hi byte)
        serial_outb(port + 3, 0x03); // 8 bits, no parity, one stop bit
        serial_outb(port + 2, 0xC7); // FIFO, 14 bytes
        serial_outb(port + 4, 0x0B); // IRQs enabled, RTS/DSR set
        inited = 1;
    }
    while (!(serial_inb(port + 5) & 0x20));
    serial_outb(port, (uint8_t)c);
}

static void serial_write(const char* s) {
    if (!s) return;
    while (*s) {
        serial_write_char(*s++);
    }
}

//klog → print
//klogf → print with formatting
//kklog → print without /n
//kklog → print with formatting without /n

void klog(const char* msg) {
    print_string(msg);
    print_char('\n');
    serial_write(msg);
    serial_write("\n");
}

void kklog(const char* msg) {
    print_string(msg);
    serial_write(msg);
}

void klog_hex(uint32_t val) {
    print_string("0x");
    print_hex(val);
    print_char('\n');
}

void klogf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[32];
    char ch;
    for (size_t i = 0; fmt[i]; i++) {
        if (fmt[i] != '%') {
            char out[2] = { fmt[i], 0 };
            klog(out);
            continue;
        }
        i++;
        switch (fmt[i]) {
            case 'd': {
                int v = va_arg(args, int);
                itoa(v, buf, 10);
                klog(buf);
                break;
            }
            case 'x': {
                int v = va_arg(args, int);
                itoa(v, buf, 16);
                klog("0x");
                klog(buf);
                break;
            }
            case 's': {
                char *s = va_arg(args, char*);
                if (!s) s = "(null)";
                klog(s);
                break;
            }
            case 'c': {
                ch = (char)va_arg(args, int);
                char out[2] = { ch, 0 };
                klog(out);
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
                    klog(buf);
                    i += 2; 
                } else {
                    klog("<?>");
                }
                break;
            }
            case '%':
                klog("%");
                break;
            default:
                klog("<?>");
                break;
        }
    }

    va_end(args);
}

void kklogf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buf[32];
    char ch;

    for (size_t i = 0; fmt[i]; i++) {
        if (fmt[i] != '%') {
            char out[2] = { fmt[i], 0 };
            kklog(out);
            continue;
        }

        i++;

        switch (fmt[i]) {
            case 'd': {
                int v = va_arg(args, int);
                itoa(v, buf, 10);
                kklog(buf);
                serial_write(buf);
                break;
            }
            case 'x': {
                int v = va_arg(args, int);
                itoa(v, buf, 16);
                //kklog("0x");
                //serial_write("0x");
                kklog(buf);
                serial_write(buf);
                break;
            }
            case 's': {
                char *s = va_arg(args, char*);
                if (!s) s = "(null)";
                kklog(s);
                serial_write(s);
                break;
            }
            case 'c': {
                ch = (char)va_arg(args, int);
                char out[2] = { ch, 0 };
                kklog(out);
                serial_write(out);
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

    va_end(args);
}
