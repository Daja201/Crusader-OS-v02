#include "klog.h"
#include "bioskbd.h"
#include "terminal.h"
#include "fs.h"
#include "rtc.h"
#include "string.h"
#include "vesa.h"
#include "commands.h"
#include "bootinfo.h"
#include "pmm.h"

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
    vesa_init_from_params(boot_fb_addr, boot_fb_width, boot_fb_height, boot_fb_bpp, boot_fb_pitch);
    gui();
    c_x = 0;
    c_y = 0;
    logo();
    extern uint8_t *block_bitmap;
    init_fs();
    verse();
    appname("TERMINAL");
    klog("\n");
    klog_yellow("CRUSADER>>> ");
    for (;;) {
        char c = bios_getchar_echo();
        terminal_key(c);
    }
}

