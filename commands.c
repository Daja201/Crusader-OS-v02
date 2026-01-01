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
    vga_init();
    klog("\n          (__) \n"
         "          (oo) \n"
         "   /-------\\/  \n"
         "  / |     ||   \n"
         " +  ||----||   \n"
         "    ^^    ^^   \n");  
    vga_init();
    klog("\n\n\n          (__) \n"
     "          (oo) \n"
     "   /-------\\/  \n"
     "  / |     ||   \n"
     " +  ||----||   \n"
     "    ^^    ^^   \n");
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
