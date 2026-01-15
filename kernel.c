#include "vga.h"
#include "klog.h"
#include "bioskbd.h"
#include "terminal.h"
#include "fs.h"


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
    terminal_init();
    klog("kernel.c OK");
    //
    extern uint8_t block_bitmap[BLOCK_BITMAP_SIZE];

    //
    init_fs();
    klog("fs.c OK");

    //r is variable i guess

    int r = sum_of_three(1, 2, 3);

    klog("variable r which equals sum_of_three equals: ");
    klog_hex(r);

    
    //TEST MORE IDE/ADATE DISK WRITE
    //uint8_t buf[512];
    //block_read(0, buf);      // přečte superblock
    //buf[0] = 0xAA;           // změní první byte
    //block_write(0, buf);     // zapíše zpět
    //TEST END
    
    
    for (;;) {
        char c = bios_getchar_echo();
        terminal_key(c);
    }
}
