#ifndef COMMANDS_H
#define COMMANDS_H

// callback command
typedef void (*cmd_func_t)(int argc, char** argv);

// commands structure lore
typedef struct {
    const char* name;
    cmd_func_t func;
} command_t;

// commands:
extern command_t commands[];
extern int command_count;

void cmd_read(int argc, char** argv);
void cmd_ls(int argc, char** argv);

#endif
