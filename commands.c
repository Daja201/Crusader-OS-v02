#include "commands.h"
#include "terminal.h"
#include "vga.h"
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
    /* Smooth vertical jumping cow:
       - interpolate many steps between top and bottom
       - slower, smoother motion (~3s each direction)
       - clear screen each frame to keep it clean */
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
    int bottom_y = screen_h - cow_lines; /* bottom so cow's last line is row 24 */

    const int total_ms = 6000;          /* round-trip ~6s */
    const int frames_per_jump = 40;     /* 40 frames per direction -> smooth */
    const int ms_per_frame = (total_ms / 2) / frames_per_jump; /* ~75ms */

    /* busy-wait delay; adjust multiplier if timing is off in your VM */
    void busy_ms(int ms) {
        volatile unsigned int iter = 8000 * ms;
        for (volatile unsigned int i = 0; i < iter; i++) asm volatile ("nop");
    }

    /* top -> bottom */
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

    /* bottom -> top */
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
