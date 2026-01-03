#include "commands.h"
#include "terminal.h"
#include "vga.h"
#include <string.h>
#include <stdio.h>
#include <cpuid.h>
#include "reboot.h"

// pure commands inside lore lol <3

void cmd_help(int argc, char** argv) {
    klog("Welcome to Crusader OS makde by David Zapletal");
    klog("Source code shoould be available on: https://github.com/Daja201/Crusader-OS-v02");
    klog("Feel free to copy and change source code for yourself.");
    klog("Run command: 'lib' for info about commands and whole system.");
}

void busy_ms(int ms) {
            volatile unsigned int iter = 8000 * ms;
            for (volatile unsigned int i = 0; i < iter; i++) asm volatile ("nop");
        }

//YAII FINALLY JUPMING COW (Thanks to Lujza <3 for whole idea)

void cmd_cow(int argc, char** argv) {
    for (int i = 0; i < 10; i++) {
        const char *cow[] = {
            "          (__) ",
            "          (oo) ",
            "   /-------\\/  ",
            "  / |     ||   ",
            " +  ||----||   ",
            "    ^^    ^^   "
        };
        const int cow_lines = sizeof(cow) / sizeof(cow[0]);
        int cow_width = 0;
        for (int i = 0; i < cow_lines; i++) {
            int l = (int)strlen(cow[i]);
            if (l > cow_width) cow_width = l;
        }

        const int screen_w = 80;
        const int screen_h = 25;
        int x = (screen_w - cow_width) / 2;
        
        if (x < 0) x = 0;
        int top_y = 1;
        int bottom_y = screen_h - cow_lines;
        const int total_ms = 6000;     
        const int frames_per_jump = 40;    
        const int ms_per_frame = (total_ms / 2) / frames_per_jump; 
        

        for (int step = 0; step <= frames_per_jump; step++) {
            int y = (top_y * (frames_per_jump - step) + bottom_y * step + frames_per_jump/2) / frames_per_jump;
            vga_init();
            for (int r = 0; r < y; r++) print_string("\n");
            for (int i = 0; i < cow_lines; i++) {
                for (int s = 0; s < x; s++) print_char(' ');
                print_string(cow[i]);
                print_string("\n");
            }
            busy_ms(ms_per_frame);
        }

        for (int step = frames_per_jump; step >= 0; step--) {
            int y = (top_y * (frames_per_jump - step) + bottom_y * step + frames_per_jump/2) / frames_per_jump;
            vga_init();
            for (int r = 0; r < y; r++) print_string("\n");
            for (int i = 0; i < cow_lines; i++) {
                for (int s = 0; s < x; s++) print_char(' ');
                print_string(cow[i]);
                print_string("\n");
            }
            busy_ms(ms_per_frame);
            
        
        }
        
    }
    return;
}

//



void cmd_cat(int argc, char** argv) {
    const char *cat =
"    /\\___/\\   \n"
"   /       \\  \n"
"  |  u   u  | \n"
"--|----*----|--\n"
"   \\   w   /       \n"
"     ======\n"
"   /       \\ __   \n"
"   |        |\\ \\   \n"
"   |        |/ /     \n"
"   |  | |   | /   \n"
"   \\ ml lm /_/      \n";
    print_string(cat);
}


void cmd_clear(int argc, char** argv) {
    vga_init();
}

void cmd_reboot(int argc, char** argv) {
    klog("reboot in process\n");
    reboot_triple_fault();
}

// comms table for functions link to comms:
command_t commands[] = {
    {"help", cmd_help},
    {"clear", cmd_clear},
    {"reboot", cmd_reboot},
    {"cow", cmd_cow},
    {"cat", cmd_cat}
};
//
int command_count = sizeof(commands)/sizeof(command_t);
