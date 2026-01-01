#include "vga.h"
#include "klog.h"

int sum_of_three(int a, int b, int c)
{
    return a + b + c;
}

void kmain()
{
    vga_init();

    klog("Kernel start OK");

    int r = sum_of_three(1, 2, 3);

    klog("Výsledek sum_of_three =");
    klog_hex(r);

    for(;;) asm("hlt");
}
