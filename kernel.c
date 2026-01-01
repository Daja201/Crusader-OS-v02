// #include "vga.h"
#include "klog.h"

//SOME RANDOM C TYPA SHII

int sum_of_three(int a, int b, int c)
{
    return a + b + c;
}

// C FUNC FOR PRINT
//For print call klog

void kmain()
{
    vga_init();

    klog("kernel.c OK");

    //r is variable i guess

    int r = sum_of_three(1, 2, 3);

    klog("variable r which equals sum_of_three equals: ");
    klog_hex(r);

    for(;;) asm("hlt");
}
