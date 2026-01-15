#include <stdint.h>
#include "klog.h"

//DE-FACTO io.h
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
//
void detect_disk() {
    uint8_t status = inb(0x1F7);
    if (status == 0xFF || status == 0x00) {
        klog("No disk detected on primary ATA");
        return;
    }

    if (status & 0x01) {
        klog("Disk error detected");
        return;
    }

    if (status & 0x40) {
        klog("Disk ready (DRDY set)");
    } else {
        klog("Disk not ready");
    }
}

void identify_disk() {
    uint16_t data[256];
    // select master
    outb(0x1F6, 0xA0);
    outb(0x1F2, 0);
    outb(0x1F3, 0);
    outb(0x1F4, 0);
    outb(0x1F5, 0);
    outb(0x1F7, 0xEC);
    while (!(inb(0x1F7) & 0x08));
    for (int i = 0; i < 256; i++) {
        uint16_t val;
        asm volatile("inw %%dx, %0" : "=a"(val) : "d"(0x1F0));
        data[i] = val;
    }

    klog("Disk IDENTIFY read OK");
}
