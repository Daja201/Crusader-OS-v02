#include "vga.h"
//#include "vga13.h"
#include "klog.h"
#include "bioskbd.h"
#include "terminal.h"
#include "fs.h"
#include "rtc.h"
#include "string.h"
#include "vesa.h"
#include "commands.h"

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

    
    //vga_init();
    
    vesa_init_from_params(boot_fb_addr, boot_fb_width, boot_fb_height, boot_fb_bpp, boot_fb_pitch);
    /*kklog_red("                                                                      ");
    kklog_red("   CCC  RRRR  U   U  SSSS   A   DDDD  EEEEE RRRR         OOO   SSSS   ");
    kklog_red("  C   C R   R U   U S      A A  D   D E     R   R       O   O S       ");
    kklog_red("  C     RRRR  U   U  SSS  AAAAA D   D EEEE  RRRR        O   O  SSS    ");
    kklog_red("  C   C R  R  U   U     S A   A D   D E     R  R        O   O     S   ");
    kklog_red("   CCC  R   R  UUU  SSSS  A   A DDDD  EEEEE R   R        OOO  SSSS    ");
    kklog_red("                                                                      ");
    kklog_red("                           Deus vult                                  ");
    kklog_red("                                                                      ");
    kklog_red("                                                                      ");
    kklog_red("                    Made by github: Daja201                           ");
    kklog_red("                                                                      ");
    */
    logo();
    /*if (boot_has_fb) {
        klog("Framebuffer detected by bootloader:");
        kklogf(" addr=0x%lx", boot_fb_addr);
        kklogf(" w=%d h=%d bpp=%d pitch=%d\n", boot_fb_width, boot_fb_height, boot_fb_bpp, boot_fb_pitch);
    } else {
        klog("No framebuffer info from bootloader; using text mode");
    }
    */

    //terminal_init();
    //klog("BOOT OK");
    //
    extern uint8_t *block_bitmap;
    //
    init_fs();
    //cmd_time();
    /*vesa_init_from_params(boot_fb_addr, boot_fb_width, boot_fb_height, boot_fb_bpp, boot_fb_pitch);
    */
    
    //if (boot_has_fb) {    
    
        /*  vesa_init_from_params(boot_fb_addr, boot_fb_width, boot_fb_height, boot_fb_bpp, boot_fb_pitch);
        for (int y = 0; y < boot_fb_height; y++) {
            for (int x = 0; x < boot_fb_width; x++) {
                vesa_putpixel(x, y, 0x000000FF); // Blue!
            }
        } */

    //kklog("FRAMEBUFFER DETECTED");
    //kklog("KERNEL BOOT OKAY");
    //kklog("WELCOME TO CRUSADER OS");
    
    //vesa_draw_hor(100, 100, 300, 0xFF0000);
    //vesa_draw_ver(100, 100, 300, 0x00FF00);
    //vesa_draw_rec(10, 10, 1580, 880, 0x00FF00);
    //vesa_draw_rec(1610, 10, 360, 880, 0x00FF00);
        //rectannle lol /*
    /*vga13_fill_rect(10, 10, 1000, 1000, 15);
    vga13_swap_buffers(); 
    //} */

    //logo();
    
    for (;;) {
        char c = bios_getchar_echo();
        terminal_key(c);
    }
}

