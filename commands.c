#include "commands.h"
#include "terminal.h"
#include "vga.h"
//#include "vga13.h"
#include <string.h>
#include <stdio.h>
#include <cpuid.h>
#include "reboot.h"
#include "diskinfo.h"
#include "fs.h"
#include "library.h"
#include "klog.h"
#include "string.h"
#include "fs.h"
#include "rtc.h"
#include "vesa.h"
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

        const int screen_w = 900;
        const int screen_h = 1600;
        int x = (screen_w - cow_width) / 2;
        
        if (x < 0) x = 0;
        int top_y = 1;
        int bottom_y = screen_h - cow_lines;
        const int total_ms = 6000;     
        const int frames_per_jump = 40;    
        const int ms_per_frame = (total_ms / 2) / frames_per_jump; 
        

        for (int step = 0; step <= frames_per_jump; step++) {
            int y = (top_y * (frames_per_jump - step) + bottom_y * step + frames_per_jump/2) / frames_per_jump;
            vesa_clear(0x000000);
            for (int r = 0; r < y; r++) vesa_print_string("\n");
            for (int i = 0; i < cow_lines; i++) {
                for (int s = 0; s < x; s++) vesa_print_char(' ');
                vesa_print_string(cow[i]);
                vesa_print_string("\n");
            }
            busy_ms(ms_per_frame);
        }

        for (int step = frames_per_jump; step >= 0; step--) {
            int y = (top_y * (frames_per_jump - step) + bottom_y * step + frames_per_jump/2) / frames_per_jump;
            vesa_clear(0x000000);
            for (int r = 0; r < y; r++) vesa_print_string("\n");
            for (int i = 0; i < cow_lines; i++) 
                for (int s = 0; s < x; s++) vesa_print_char(' ');
                vesa_print_string(cow[i]);
                vesa_print_string("\n");
            }
            busy_ms(ms_per_frame);
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
    vesa_print_string(cat);
}

void cmd_ld(int argc, char** argv) {
    detect_disk();
    identify_disk();
}

void cmd_clear(int argc, char** argv) {
    vesa_clear(0x000000);
}

void cmd_reboot(int argc, char** argv) {
    klog("reboot in process\n");
    reboot_triple_fault();
}

/*void cmd_runtest(int argc, char** argv) {
    klog("Trying runtest...\n");
    runtest_program();
} */

void cmd_lib(int argc, char** argv) {
    klog("Welcome to library of Crusader OS:\n");
    library();
}

void cmd_read(int argc, char** argv) {
    if (argc < 2) {
        klog("Usage: read <filename>");
        return;
    }
    inode_t root;
    read_inode(0, &root);

    int inode_num = dir_lookup(&root, argv[1]);
    if (inode_num < 0) {
        klog("Error: File not found");
        return;
    }
    inode_t file_node;
    read_inode(inode_num, &file_node);
    uint8_t buf[32768];
    uint32_t to_read = file_node.size;
    if (to_read > sizeof(buf)) {
        to_read = sizeof(buf); 
    }
    int bytes_read = fs_read(inode_num, &file_node, 0, to_read, buf);
    if (bytes_read <= 0) {
        klog("Error: Could not read file (or empty)");
        return;
    }
    for (int i = 0; i < bytes_read; i++) {
        if (buf[i] == '\0') break;
        char ch[2] = {buf[i], '\0'};
        vesa_print_string(ch);
    }
    vesa_print_string("\n");
}

void cmd_ls(int argc, char** argv) {
    inode_t root;
    read_inode(0, &root); 
    uint8_t buf[32678];
    struct dirent {
        uint32_t inode;
        char name[28]; //4 + 28
        //char tag[28];
    };
    int entry_count = 512 / sizeof(struct dirent);
    klog("Files in root directory:");
    //klog("777");

    for (int b = 0; b < 12; b++) {
        uint32_t block_lba = root.direct[b];
        if (block_lba == 0) break; 
        block_read(block_lba, buf);
        struct dirent* entries = (struct dirent*)buf;
        for (int i = 0; i < entry_count; i++) {
            if (entries[i].inode != 0) {
                inode_t file_node;
                klog("  ");
                klog(entries[i].name);
                klog(" - ");
                klog(file_node.main_tag);
                klog("\n");
                //CHRIST IS MESSIAH
            }
        }
    }
}
//delete func
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
    const char* wr = "wr";
    const char* filename = argv[1]; 
    const char* data = argv[2];
    //NEEDS TO CREATE INODE before wiritng in it
    uint32_t inode = fs_create_file(filename, wr);
    if ((int32_t)inode < 0) {
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
/*
void cmd_gui(int argc, char** argv) {
    /* Prefer VESA LFB if available 
    if (boot_has_fb) {
        vesa_init_from_params(boot_fb_addr, boot_fb_width, boot_fb_height, boot_fb_bpp, boot_fb_pitch);
        vesa_demo();
    } else {
        // fallback to mode13 demo if bootloader set it 
        vga13_init();
        vga13_demo();
    }
}
*/
void cmd_format(int argc, char** argv) {
    format_fs();
}
/*
void cmd_test() {
    vesa_clear(0x000000);
    for (int b = 0; b < 201 ; b++) {
        //for (int b = 0; b < 10; b++) {
        klog("C");
        //}
    }
}
*/
void cmd_find(int argc, char** argv) {
    if (argc < 2) {
        kklogf("usage: find <tag>\n");
        return;
    }
    const char* tag = argv[1];
    uint32_t results[32];
    int found_count = fs_find_by_tag(tag, results, 32);
    if (found_count <= 0) {
            kklogf("No files found with tag: %s", tag);
        } else {
            kklogf("Found %d files:", found_count);
        for (int i = 0; i < found_count; i++) {
            kklogf(" - Inode: %d", results[i]);
        }
    }
    klog("\n");
}

// comms table for functions link to comms:
command_t commands[] = {
    {"help", cmd_help},
    {"clear", cmd_clear},
    {"reboot", cmd_reboot},
    {"cow", cmd_cow},
    {"cat", cmd_cat},
    {"ld", cmd_ld},
    {"fd", cmd_find},
    {"read", cmd_read},
    {"ls", cmd_ls},
    {"lib", cmd_lib},
    {"wr", cmd_wr},
    {"dl", cmd_dl},
    {"time", cmd_time},
    //{"gui", cmd_gui},
    //{"test", cmd_test},
    {"format", cmd_format}
};
//
int command_count = sizeof(commands)/sizeof(command_t);
