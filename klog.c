#include "vga.h"
#include <stdint.h>

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
