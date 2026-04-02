#include "commands.h"
#include "terminal.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
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
#include "pmm.h"

void cmd_help(int argc, char** argv) {
    kklog_red("Welcome to Crusader OS made by David Zapletal");
    kklog_red("Source code shoould be available on: https://github.com/Daja201/Crusader-OS-v02");
    kklog_red("Feel free to copy and change source code for yourself.");
    kklog_red("Run command: 'lib' for info about commands and whole system.");
}

void busy_ms(int ms) {
    volatile unsigned int iter = 8000 * ms;
    for (volatile unsigned int i = 0; i < iter; i++) asm volatile ("nop");
}

//YAII FINALLY JUPMING COW (Thanks to Lujza <3 for whole idea)
void cmd_cow(int argc, char** argv) {
    const char *cow[] = {
        "          (__) ",
        "          (oo) ",
        "   /-------\\/  ",
        "  / |     ||   ",
        " +  ||----||   ",
        "    ^^    ^^   "
    };
    const int cow_lines = 6;
    const int char_w = 8;
    const int char_h = 8;
    
    // Calculate cow dimensions in pixels
    int cow_width_px = 15 * char_w; // "          (__) " is 15 chars
    
    // screen_h is likely fb_height (e.g., 768 or 1024)
    // Based on your vesa.c, your 'terminal' width is 952
    const int screen_w = 952;
    const int screen_h = 600; // Adjust to your actual resolution height

    int x_start = (screen_w - cow_width_px) / 2;
    if (x_start < 0) x_start = 0;

    int top_y = 10;
    int bottom_y = screen_h - (cow_lines * char_h) - 10;
    
    const int frames_per_jump = 20;    
    const int ms_per_frame = 30; 

    for (int iteration = 0; iteration < 10; iteration++) {
        for (int step = 0; step <= frames_per_jump; step++) {
            int current_y = top_y + ((bottom_y - top_y) * step / frames_per_jump);
            vesa_clear(0x000000);
            for (int i = 0; i < cow_lines; i++) {
                int line_y = current_y + (i * char_h);
                for (int j = 0; cow[i][j] != '\0'; j++) {
                    vesa_draw_char(cow[i][j], x_start + (j * char_w), line_y, 0xFFFFFF, 0x000000);
                }
            }
            busy_ms(ms_per_frame);
        }
        for (int step = frames_per_jump; step >= 0; step--) {
            int current_y = top_y + ((bottom_y - top_y) * step / frames_per_jump);
            vesa_clear(0x000000);    
            for (int i = 0; i < cow_lines; i++) {
                int line_y = current_y + (i * char_h);
                for (int j = 0; cow[i][j] != '\0'; j++) {
                    vesa_draw_char(cow[i][j], x_start + (j * char_w), line_y, 0xFFFFFF, 0x000000);
                }
            }
            busy_ms(ms_per_frame);
        }
    }
}

void cmd_mem() {
    uint32_t free_kb = pmm_count_mem();
    klogf("Volna fyzicka pamet: %d KB (%d MB)\n", free_kb, free_kb / 1024);
}

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
    kklog("reboot in process\n");
    reboot_triple_fault();
    
}

void cmd_lib(int argc, char** argv) {
    library();
}

void cmd_read(int argc, char** argv) {
    if (argc < 2) {
        kklog("Usage: read <filename>");
        return;
    }
    inode_t root;
    read_inode(0, &root);

    int inode_num = dir_lookup(&root, argv[1]);
    if (inode_num < 0) {
        kklog("Error: File not found");
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
        kklog("Error: Could not read file (or empty)");
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
        char name[28];
    };
    int entry_count = 512 / sizeof(struct dirent);
    kklog("Files in root directory:");
    for (int b = 0; b < 12; b++) {
        uint32_t block_lba = root.direct[b];
        if (block_lba == 0) break; 
        block_read(block_lba, buf);
        struct dirent* entries = (struct dirent*)buf;
        for (int i = 0; i < entry_count; i++) {
            if (entries[i].inode != 0) {
                inode_t file_node;
                kklog("  ");
                klog(entries[i].name);
                klog(" - ");
                klog(file_node.main_tag);
                klog("\n");
            }
        }
    }
}

void cmd_dl(int argc, char** argv) {
    if (argc < 2) {
        kklog("usage: dl <file>");
        return;
    }
    if (fs_delete_file(argv[1]) < 0) {
        kklog("dl: failed");
        return;
    }
    kklog("file deleted");
}

void cmd_wr(int argc, char** argv) {
    if (argc < 3) {
        kklog("Usage: wr <filename> <data>");
        return;
    }
    const char* wr = "wr";
    const char* filename = argv[1]; 
    const char* data = argv[2];
    uint32_t inode = fs_create_file(filename, wr);
    if ((int32_t)inode < 0) {
        kklog("file creation failed");
        return;
    }
    int written = fs_write(inode, (const uint8_t*)data, strlen(data));
    if (written < 0) {
        kklog("write failed");
    } else {
        kklog("write successful");
    }
}

void cmd_time(int argc, char** argv) {
    int year, month, day;
    int hour, min, sec;
    rtc_get_datetime(&year, &month, &day, &hour, &min, &sec);
    char b[8];
    klog_green("RTC: ");
    itoa(year, b, 10); klog_green(b);;itoa(month, b, 10);klog_green(" "); klog_green(b);itoa(day, b, 10);klog_green(" "); klog_green(b); klog_green(" ");
    itoa(hour, b, 10); klog_green(b); klog_green(":");
    itoa(min, b, 10); klog_green(b); klog_green(":"); 
    itoa(sec, b, 10); klog_green(b);
    klog_green("\n");
}

void cmd_note(int argc, char** argv) {
    if (argc < 3) {
        kklog("usage: note <c/d> <name> <content>\n");
        return;
    }
    char action = argv[1][0];
    const char* name = argv[2];
    if (action == 'c') {
        if (argc < 4) {
            kklog("usage: note c <name> <content>\n");
            return;
        }
        const char* content = argv[3];
        new_note(name, content);
    } 
    else if (action == 'd') {
        if (strcmp(name, "all") == 0) {
            delete_all_notes();
        } 
        else {
            delete_note(name);
        }
    } 
    else {
        kklog("usage: note <c/d> <name> <content>\n"); 
    }
}

void cmd_format(int argc, char** argv) {
    format_fs();
}

void cmd_usedisk(int argc, char** argv) {
    if (argc < 2) {
        kklog("Usage: use <disk_id>\n");
        return;
    }
    int drive_id = argv[1][0] - '0'; 
    if (fs_change_drive(drive_id) == 0) {
        kklogf("Switched to disk %d\n", drive_id);
    } else {
        kklog("Error: Invalid disk ID\n");
    }
}

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
    {"format", cmd_format},
    {"note", cmd_note},
    {"use", cmd_usedisk},
    {"mem", cmd_mem}
};

int command_count = sizeof(commands)/sizeof(command_t);