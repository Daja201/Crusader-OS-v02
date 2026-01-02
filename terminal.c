#include "terminal.h"
#include "commands.h"
#include "vga.h"
#include <string.h>
static char cmd_buf[CMD_BUF_SIZE];
static int cmd_len = 0;

void terminal_init() {
    klog("Crusader>>> ");
}
void execute_command(char* line) {
    char* argv[8];
    int argc = 0;
    char* p = line;

    while (*p && argc < 8) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = 0;
    }

    for (int i = 0; i < command_count; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].func(argc, argv);
            return;
        }
    }
    klog("sorry buddy we don't know this one\n");
}
void terminal_key(char c) {
    if (c == '\n') {
        cmd_buf[cmd_len] = 0;
        execute_command(cmd_buf);
        cmd_len = 0;
        klog("comm> ");
        return;
    }

    if (c == '\b') {
        if (cmd_len > 0) {
            cmd_len--;
            vga_backspace();
        }
        return;
    }

    if (cmd_len < CMD_BUF_SIZE - 1) {
        cmd_buf[cmd_len++] = c;
    //    char s[2] = {c, 0};
    //    klog(s);
    }
}
