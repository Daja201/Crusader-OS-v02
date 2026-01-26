#include "runtest.h"
#include "fs.h"
#include "klog.h"
#include <stdint.h>
#include "string.h"

void runtest_program(void) {
    //WRITE YOUR RUNTEST CODE:
    uint32_t inode = fs_create_file("test.tf");
    const char* msg = "LOG01-This log is written by the operating system itself by runtest.";
    int written = fs_write(inode, (const uint8_t*)msg, strlen(msg));
 

    if (written < 0) {
        klog("fs_write failed");
    }
    else {

    }

    //EXIT
}