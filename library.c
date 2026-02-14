#include "library.h"
#include "fs.h"
#include "klog.h"
#include "vga.h"
void library(void){
    vga_init();
    //
    klog("test library ran from library.c");
    klog("This is an simple operating system made by David Zapletal. Github: Daja201");
    klog("Here is simple library for using this operating system.");
    //
    klog("  BASIC:");
    klog("help - Use help for some basic info");
    klog("clear - use clear to clear display");
    klog("cow - Use cow to make an cow jump through your window");
    klog("cat - Use to print cute cat");
    klog("reboot - use reboot to reboot using triple fault");
    klog("lib - bro u are using lib rn you should know what it does");
    //
    klog("  FS:");
    klog("ld - use ld to detect and check drives");
    klog("read - use read to read from some files");
    klog("ls - use ls to read all files from directory");
    klog("dl - dl deletes file, use dl <file>");
    klog("wr - writes into file, use wr <file> <content>");
    //
    klog("  ADVANCED:");
    klog("rt - use runtest to run runtest.c and make some runtests");
    //
}