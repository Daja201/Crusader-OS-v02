#include "commands.h"
#include "terminal.h"
#include "vga.h"
#include <string.h>
#include <stdio.h>
#include <cpuid.h>
#include "reboot.h"
#include "diskinfo.h"
#include "runtest.h"
#include "fs.h"
#include "library.h"
#include "klog.h"

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

void cmd_ld(int argc, char** argv) {
    detect_disk();
    identify_disk();
}

void cmd_clear(int argc, char** argv) {
    vga_init();
}

void cmd_reboot(int argc, char** argv) {
    klog("reboot in process\n");
    reboot_triple_fault();
}

void cmd_runtest(int argc, char** argv) {
    klog("Trying runtest...\n");
    runtest_program();
}

void cmd_lib(int argc, char** argv) {
    klog("Welcome to library of Crusader OS:\n");
    library();
}

void cmd_read(int argc, char** argv) {
    if (argc < 2) {
        klog("Usage: read <filename>");
        return;
    }
    
    // Get root directory inode
    inode_t root;
    read_inode(0, &root);
    
    // Look up file in root directory
    int inode_num = dir_lookup(&root, argv[1]);
    if (inode_num < 0) {
        klog("Error: File not found");
        return;
    }
    
    // Read the file
    uint8_t buf[512];
    int bytes_read = fs_read(inode_num, buf, 512);
    
    if (bytes_read <= 0) {
        klog("Error: Could not read file");
        return;
    }
    
    // Print the file contents
    for (int i = 0; i < bytes_read; i++) {
        if (buf[i] == '\0') break;
        char ch[2] = {buf[i], '\0'};
        print_string(ch);
    }
    print_string("\n");
}

void cmd_ls(int argc, char** argv) {
    inode_t root;
    read_inode(0, &root);
    
    uint8_t buf[512];
    block_read((uint32_t)root.direct[0], buf);
    
    struct dirent {
        uint32_t inode;
        char name[28];
    };
    
    struct dirent* entries = (struct dirent*)buf;
    int entry_count = 512 / sizeof(struct dirent);
    
    klog("Files in root directory:");
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].inode != 0) {
            print_string("  ");
            print_string(entries[i].name);
            print_string("\n");
        }
    }
}

void cmd_dl(int argc, char** argv) {
    if (argc < 2) {
        klog("usage: dl <file>");
        return;
    }
    if (fs_delete_file(argv[1]) < 0) {
        klog("dl: failed");
        return;
    }
    klog("file deleted");
}

void cmd_wr(int argc, char** argv) {
    if (argc < 3) {
        klog("Usage: wr <filename> <data>");
        return;
    }
    const char* filename = argv[1]; 
    const char* data = argv[2];
    //NEEDS TO CREATE INODE before wiritng in it
    uint32_t inode = fs_create_file(filename);
    if (inode == 0) {
        klog("file creation failed");
        return;
    }
    int written = fs_write(inode, (const uint8_t*)data, strlen(data));
    if (written < 0) {
        klog("write failed");
    } else {
        klog("write successful");
    }
}

void cmd_time(int argc, char** argv) {
    int year, month, day;
    int hour, min, sec;
    rtc_get_datetime(&year, &month, &day, &hour, &min, &sec);
    char b[8];
    klog("RTC: ");
    itoa(year, b, 10); kklog(b);;itoa(month, b, 10);kklog(" "); kklog(b);itoa(day, b, 10);kklog(" "); kklog(b); kklog(" ");
    itoa(hour, b, 10); kklog(b); kklog(":");
    itoa(min, b, 10); kklog(b); kklog(":"); 
    itoa(sec, b, 10); klog(b);
}

// comms table for functions link to comms:
command_t commands[] = {
    {"help", cmd_help},
    {"clear", cmd_clear},
    {"reboot", cmd_reboot},
    {"cow", cmd_cow},
    {"cat", cmd_cat},
    {"ld", cmd_ld},
    {"rt", cmd_runtest},
    {"read", cmd_read},
    {"ls", cmd_ls},
    {"lib", cmd_lib},
    {"wr", cmd_wr},
    {"dl", cmd_dl},
    {"time", cmd_time}
};
//
int command_count = sizeof(commands)/sizeof(command_t);
