#include "vga.h"
#include <stdint.h>
#include "klog.h"
#include "string.h"
#include <stdarg.h>

//print for chars

void klog(const char* msg)
{
    print_string(msg);
    print_char('\n');
}

//print for variables
void klog_hex(uint32_t val)
{
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

        i++; // přeskoč %

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
