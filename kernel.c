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


void system_main_task() {
    int last_sec = -1;
    for (;;) {
        int needs_redraw = 0; // Vlajka, zda máme překreslit monitor

        // 1. Zpracování klávesnice
        if (bios_has_char()) { 
            char c = bios_getchar_echo(); 
            if (c != 0) {
                terminal_key(c);
                needs_redraw = 1; // Napsali jsme znak, chceme hned překreslit!
            }
        }
        
        // 2. Zpracování času
        int y, m, d, h, min, sec;
        rtc_get_datetime(&y, &m, &d, &h, &min, &sec);
        if (sec != last_sec) {
            last_sec = sec;
            clock(); 
            free_ram();
            needs_redraw = 1; // Změnil se čas, chceme hned překreslit!
        }

        // 3. Překreslení obrazovky (mimo IF se sekundami!)
        if (needs_redraw) {
            cursor('d');  // Vykreslí kurzor na aktuální pozici
            vesa_swap();  // Pošle obraz na monitor
        }
        
        asm volatile("pause"); // Šetříme CPU cykly
    }
}

void kmain(unsigned long mb_magic, unsigned long mb_info) {
    // 1. Základní nastavení a zjištění paměti
    parse_multiboot((uint32_t)mb_magic, (uint32_t)mb_info);
    
    pmm_init(); 
    pmm_init_region(0x200000, 0x200000); // Tady dáváme PMM paměť k rozdávání

    // 2. Přerušení a VESA
    init_idt();
    vesa_init_from_params(boot_fb_addr, boot_fb_width, boot_fb_height, boot_fb_bpp, boot_fb_pitch);
    
    // --- TVŮJ PŮVODNÍ GUI KÓD, KTERÝ CHYBĚL ---
    extern void init_paging(uint32_t, uint32_t, uint32_t, uint32_t);
    init_paging(boot_fb_addr, boot_fb_width, boot_fb_height, boot_fb_bpp);
    
    gui();
    c_x = 0;
    c_y = 0;
    logo();
    
    init_fs();
    verse();
    appname("TERMINAL");
    klog("\n");
    klog_yellow("CRUSADER>>> ");
    klog_status("BOOTED");
    // ------------------------------------------

    // 3. Spuštění Multitaskingu
    timer_init(100); 
    init_multitasking();

    // Vytvoříme hlavní proces (ten se stará o klávesnici a hodiny)
    create_task(system_main_task); 

    klog_status("MULTITASKING STARTED");
    
    // Povolíme přerušení - od teď procesor skáče do isr32 a přepíná
    asm volatile("sti");
    
    // Náš Idle task (hlavní vlákno kernelu), které jen spí
    while (1) {
        asm volatile("hlt");
    }
}