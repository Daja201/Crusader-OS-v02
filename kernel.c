#include "vga.h"
#include "vga13.h"
#include "klog.h"
#include "bioskbd.h"
#include "terminal.h"
#include "fs.h"
#include "rtc.h"
#include "string.h"

void kmain(unsigned long mb_magic, unsigned long mb_info) {
    
    parse_multiboot((uint32_t)mb_magic, (uint32_t)mb_info);
    uint32_t mb_flags = *(uint32_t*)mb_info;
    if (mb_flags & 4) {
        char *cmdline = (char*)(*(uint32_t*)(mb_info + 16));
        if (cmdline) {
            for (int i = 0; cmdline[i] != '\0'; i++) {
                if (cmdline[i] == 't' && cmdline[i+1] == 'e' && cmdline[i+2] == 'x' && cmdline[i+3] == 't') {
                    boot_has_fb = 0;
                    break;
                }
            }
        }
    }
    //

    vga_init();

    /*if (boot_has_fb) {
        klog("Framebuffer detected by bootloader:");
        kklogf(" addr=0x%lx", boot_fb_addr);
        kklogf(" w=%d h=%d bpp=%d pitch=%d\n", boot_fb_width, boot_fb_height, boot_fb_bpp, boot_fb_pitch);
    } else {
        klog("No framebuffer info from bootloader; using text mode");
    }
    */

    terminal_init();
    //klog("BOOT OK");
    //
    extern uint8_t *block_bitmap;
    //
    init_fs();
    int year, month, day;
    int hour, min, sec;
    rtc_get_datetime(&year, &month, &day, &hour, &min, &sec);
    char b[8];
    /*klog("");
    klog("RTC: ");
    itoa(year, b, 10); kklog(b);;itoa(month, b, 10);kklog(" "); kklog(b);itoa(day, b, 10);kklog(" "); kklog(b); kklog(" ");
    itoa(hour, b, 10); kklog(b); kklog(":");
    itoa(min, b, 10); kklog(b); kklog(":"); 
    itoa(sec, b, 10); klog(b);
    //vesa_init_from_params(boot_fb_addr, boot_fb_width, boot_fb_height, boot_fb_bpp, boot_fb_pitch);
    */

    if (boot_has_fb) {    
        vesa_init_from_params(boot_fb_addr, boot_fb_width, boot_fb_height, boot_fb_bpp, boot_fb_pitch);
    
        /*  vesa_init_from_params(boot_fb_addr, boot_fb_width, boot_fb_height, boot_fb_bpp, boot_fb_pitch);
        for (int y = 0; y < boot_fb_height; y++) {
            for (int x = 0; x < boot_fb_width; x++) {
                vesa_putpixel(x, y, 0x000000FF); // Blue!
            }
        } */

        //klog("Framebuffer detected by bootloader:");
        //vesa_draw_char('N', 100, 100, 0xFFFFFF, 0x0000FF);
        klog("CD");

    }

    for (;;) {
        char c = bios_getchar_echo();
        terminal_key(c);
    }
}
