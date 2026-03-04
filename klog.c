#include "vga.h"
#include <stdint.h>
#include "klog.h"
#include "string.h"
#include <stdarg.h>

//klog → print
//klogf → print with formatting
//kklog → print without /n
//kklog → print with formatting without /n

void klog(const char* msg) {
    print_string(msg);
    print_char('\n');
}

void kklog(const char* msg) {
    print_string(msg);
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
                break;
            }
            case 'x': {
                int v = va_arg(args, int);
                itoa(v, buf, 16);
                kklog("0x");
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

    va_end(args);
}
