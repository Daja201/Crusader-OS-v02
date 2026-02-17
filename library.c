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
    klog("help - some basic info");
    klog("clear -  clears display");
    klog("cow - makes an cow jump through your window");
    klog("cat - prints cute cat");
    klog("reboot - reboots using triple fault");
    klog("lib - bro u are using lib rn you should know what it does");
    //
    klog("  FS:");
    klog("ld - detects and checks drives");
    klog("read - reads from some files");
    klog("ls - reads all files from directory");
    klog("dl - deletes file, use dl <file>");
    klog("wr - writes into file, use wr <file> <content>");
    //
    klog("  ADVANCED:");
    klog("rt - runs runtest.c and make some runtests");
    klog("time - shows time from RealTimeClock");
    //
}