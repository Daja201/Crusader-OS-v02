#include "commands.h"
#include "terminal.h"
#include <string.h>

// pure commands inside lore lol <3

void cmd_help(int argc, char** argv) {
    klog("Available commands:\n");
    for (int i = 0; i < command_count; i++) {
        klog(commands[i].name);
        klog("\n");
    }
}

void cmd_cow(int argc, char** argv) {
    /* Jumping cow: alternate between top-centered and bottom-centered
       positions for ~5 seconds, clearing screen between frames. */
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
    int top_y = (screen_h - cow_lines) / 4;    /* upper quarter (centered near top) */
    int bottom_y = (screen_h - cow_lines) * 3 / 4; /* lower quarter (centered near bottom) */

    const int total_ms = 5000;
    const int toggles = 20; /* number of frames (top/bottom alternations) */
    const int ms_per_frame = total_ms / toggles;

    /* busy-wait delay */
    void busy_ms(int ms) {
        volatile unsigned int iter = 2500 * ms;
        for (volatile unsigned int i = 0; i < iter; i++) asm volatile ("nop");
    }

    for (int t = 0; t < toggles; t++) {
        int y = (t % 2 == 0) ? top_y : bottom_y;
        vga_init(); /* fully clear screen */
        /* move down y lines */
        for (int r = 0; r < y; r++) print_string("\n");
        for (int i = 0; i < cow_lines; i++) {
            for (int s = 0; s < x; s++) print_char(' ');
            print_string(cow[i]);
            print_string("\n");
        }
        busy_ms(ms_per_frame);
    }
}

void cmd_clear(int argc, char** argv) {
    vga_init();
}

void cmd_reboot(int argc, char** argv) {
    klog("reboot in process\n");
    //pls add reboot code ):
}

// comms table for functions link to comms:
command_t commands[] = {
    {"help", cmd_help},
    {"clear", cmd_clear},
    {"reboot", cmd_reboot},
    {"cow", cmd_cow}
};
//
int command_count = sizeof(commands)/sizeof(command_t);
