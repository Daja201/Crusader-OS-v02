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
#include "bioskbd.h"
#include "idt.h"
#include "task.h"
volatile uint32_t timer_ticks = 0;

void timer_init(uint32_t frequency) {
    uint32_t divisor = 1193182 / frequency;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

//
volatile int counter_a = 0;
volatile int counter_b = 0;
uint32_t stack1[8192];
uint32_t stack2[8192];
void task1() {
    while (1) {
        counter_a++;
        asm volatile("pause" ::: "memory"); 
    }
}
void task2() {
    while (1) {
        counter_b++;
        asm volatile("pause" ::: "memory"); 
    }
}
//

void kmain(unsigned long mb_magic, unsigned long mb_info) {
    parse_multiboot((uint32_t)mb_magic, (uint32_t)mb_info);
    init_idt();
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
    extern void init_paging(uint32_t, uint32_t, uint32_t, uint32_t);
    init_paging(boot_fb_addr, boot_fb_width, boot_fb_height, boot_fb_bpp);
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
    int last_sec = -1;
    klog_status("BOOTED");
    load_notes_from_disk();
    timer_init(1);
    init_multitasking();
    create_task(task1, &stack1[8192]);
    create_task(task2, &stack2[8192]);
    asm volatile("sti");
    for (;;) {
        int needs_redrawing = 0;
        if (bios_has_char()) { 
            char c = bios_getchar_echo(); 
            if (c != 0) {
                terminal_key(c);
                needs_redrawing = 1;
            }
            needs_redrawing = 1;
        }
        int year, month, day, hour, min, sec;
        rtc_get_datetime(&year, &month, &day, &hour, &min, &sec);
        if (sec != last_sec) {
            last_sec = sec;
            needs_redrawing = 1;
        }
        if (needs_redrawing) {
            clock(); 
            free_ram();
            vesa_swap(); 
            cursor('d');
        }
        asm volatile("hlt");
    }
}