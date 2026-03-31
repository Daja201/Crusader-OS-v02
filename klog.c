#include "vga.h"
#include "vesa.h"
#include <stdint.h>
#include "klog.h"
#include "string.h"
#include <stdarg.h>
#include "commands.h"
static uint16_t x_letter;
static uint16_t y_letter;

static void vprintf_internal(const char *fmt, va_list args) {
    char buf[32];
    char ch;
    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] != '%') {
            char out[2] = { fmt[i], 0 };
            klog_green(out);
            continue;
        }
        i++;
        switch (fmt[i]) {
            case 'd': {
                int val = va_arg(args, int);
                itoa(val, buf, 10);
                klog_green(buf);
                break;
            }
            case 'x': {
                uint32_t val = va_arg(args, uint32_t);
                itoa(val, buf, 16);
                klog_green(buf);
                break;
            }
            case 's': {
                char *s = va_arg(args, char*);
                if (!s) s = "(null)";
                klog_green(s);
                break;
            }
            case 'c': {
                ch = (char)va_arg(args, int);
                char out[2] = { ch, 0 };
                klog_green(out);
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
}

void klog(const char* msg) {
    while (*msg != '\0') {
        char c = *msg;

        if (c == '\n') {
            x_letter = 0;
            y_letter += 8;
        } else {
            vesa_draw_char(c, x_letter, y_letter, 0x00FF00, 0x000000);
            x_letter += 8;
            if (x_letter > 1080) {
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
}

void kklogf(const char *fmt, ...) {
    klogf(fmt);  
}

//COLOUR FONTS:

static void vprintf_internal_green_kk(const char *fmt, va_list args) {
    char buf[32];
    char ch;
    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] != '%') {
            char out[2] = { fmt[i], 0 };
            klog_green(out);
            continue;
        }
        i++;
        switch (fmt[i]) {
            case 'd': {
                int val = va_arg(args, int);
                itoa(val, buf, 10);
                kklog_green(buf);
                break;
            }
            case 'x': {
                uint32_t val = va_arg(args, uint32_t);
                itoa(val, buf, 16);
                klog_green(buf);
                break;
            }
            case 's': {
                char *s = va_arg(args, char*);
                if (!s) s = "(null)";
                kklog_green(s);
                break;
            }
            case 'c': {
                ch = (char)va_arg(args, int);
                char out[2] = { ch, 0 };
                kklog_green(out);
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
                    klog_green(buf);
                    i += 2; 
                } else {
                    kklog_green("<?>");
                }
                break;
            }
            case '%':
                kklog_green("%");
                break;

            default:
                kklog_green("<?>");
                break;
        }
    }
}

void logo() {
    klog_red("                                                                                                           ");
    cmd_time();
    klog_red("     __                                                                                                    ");
    drives();
    kklog_red("   ,/ _~.                          |\\                    ,-||-,     -_-/      ------                       Welcome to Crusader OS      ");
    kklog_red("  (' /|                       _     \\\\                  ('|||  )   (_ /         ||                         An hobby operating system   ");
    kklog_red(" ((  ||   ,._-_ \\\\ \\\\  _-_,  < \\,  / \\\\  _-_  ,._-_    (( |||--)) (_ --_   |    ||    |                    made by:                    ");
    kklog_red(" ((  ||    ||   || || ||_.   /-|| || || || \\\\  ||      (( |||--))   --_ )  |====[]====|                    David Zapletal              ");
    kklog_red("  ( / |    ||   || ||  ~ || (( || || || ||/    ||       ( / |  )   _/  ))  |    ||    |                                                ");
    kklog_red("   \\____-  \\\\,  \\\\/\\\\ ,-_-   \\/\\\\  \\\\/  \\\\,/   \\\\,       -____-   (_-_-         ||                                                     ");
    kklog_red("                                                                              ------                                                   ");
    kklog_red("                                DEUS VULT                                                                                              ");
    kklog_red("                                                                                                                                       ");
    kklog_red("                          Made by github:Daja201                                                                                       ");
    kklog_red("                                                                                                                                       ");
}

void klogf_green(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf_internal(fmt, args);
    va_end(args);
}

void kklogf_green(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf_internal_green_kk(fmt, args);
    va_end(args);
}

void klog_green(const char* msg) {
    while (*msg != '\0') {
        char c = *msg;
        if (c == '\n') {
            x_letter = 0;
            y_letter += 8;
        } else {
            vesa_draw_char(c, x_letter, y_letter, 0x000000, 0x00FF00);
            x_letter += 8;
            if (x_letter > 1080) {
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

void kklog_green(const char* msg) {
    while (*msg != '\0') {
        char c = *msg;
        if (c == '\n') {
            x_letter = 0;
            y_letter += 8;
        } else {
            vesa_draw_char(c, x_letter, y_letter, 0x000000, 0x00FF00);
            x_letter += 8;
            if (x_letter > 1080) {
                x_letter = 0;
                y_letter += 8;
            }
        }
        if (y_letter >= 900) {
            y_letter = 0;
        }
        msg++; 
    }
    klog("\n");
}

void klog_red(const char* msg) {
    while (*msg != '\0') {
        char c = *msg;
        if (c == '\n') {
            x_letter = 0;
            y_letter += 8;
        } else {
            vesa_draw_char(c, x_letter, y_letter, 0xFFFFFF, 0xFF0000);
            x_letter += 8;
            if (x_letter > 1080) {
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

void kklog_red(const char* msg) {
    while (*msg != '\0') {
        char c = *msg;
        if (c == '\n') {
            x_letter = 0;
            y_letter += 8;
        } else {
            vesa_draw_char(c, x_letter, y_letter, 0xFFFFFF, 0xFF0000);
            x_letter += 8;
            if (x_letter > 1080) {
                x_letter = 0;
                y_letter += 8;
            }
        }
        if (y_letter >= 900) {
            y_letter = 0;
        }
        msg++;
    }
    klog("\n");
}