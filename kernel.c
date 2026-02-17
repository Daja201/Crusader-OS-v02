#include "vga.h"
#include "klog.h"
#include "bioskbd.h"
#include "terminal.h"
#include "fs.h"
#include "rtc.h"

//SOME RANDOM C TYPA SHII

//int sum_of_three(int a, int b, int c)
//{
//    return a + b + c;
//}

// C FUNC FOR PRINT
//For print call klog

void kmain()
{
    vga_init();
    terminal_init();
    klog("BOOT OK");
    //
    extern uint8_t *block_bitmap;
    //
    init_fs();
    //klog("fs.c OK");
    //r is variable i guess
    //int r = sum_of_three(1, 2, 3);
    //klog("variable r which equals sum_of_three equals: ");
    //klog_hex(r);
    //
    int year, month, day;
    int hour, min, sec;
    rtc_get_datetime(&year, &month, &day, &hour, &min, &sec);
    char b[8];
    klog("RTC: ");
    itoa(year, b, 10); kklog(b);;itoa(month, b, 10);kklog(" "); kklog(b);itoa(day, b, 10);kklog(" "); kklog(b); kklog(" ");
    itoa(hour, b, 10); kklog(b); kklog(":");
    itoa(min, b, 10); kklog(b); kklog(":"); 
    itoa(sec, b, 10); klog(b);

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
