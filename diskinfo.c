//SOLVE ERROR WHEN USING >32 MB drives i guess, 3rd klog in output

#include <stdint.h>
#include "klog.h"
#include "fs.h"

// use global superblock from fs.c
extern superblock_t g_superblock;


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

    // Determine total sectors: prefer words 100..103 (LBA48) if non-zero, else words 60..61 (LBA28)
    uint64_t sectors = ((uint64_t)data[100]) |
                       ((uint64_t)data[101] << 16) |
                       ((uint64_t)data[102] << 32) |
                       ((uint64_t)data[103] << 48);
    if (sectors == 0) {
        sectors = ((uint64_t)data[61] << 16) | (uint64_t)data[60];
    }

    if (sectors == 0) {
        klog("size: unknown (IDENTIFY returned 0)");
        klog("FS superblock total_blocks:");
        klogf("%llu", (unsigned long long)g_superblock.total_blocks);
    } else {
        // print sectors
        char tmp[32]; int tp = 0;
        uint64_t tv = sectors;
        if (tv == 0) tmp[tp++] = '0';
        while (tv) { tmp[tp++] = '0' + (tv % 10); tv /= 10; }
        for (int i = 0; i < tp; ++i) tmp[i] = tmp[i]; // no-op to keep buffer
        // reverse into buf
        char buf[32]; for (int i = 0; i < tp; ++i) buf[i] = tmp[tp-1-i]; buf[tp] = '\0';
        klog("size (sectors):"); klog(buf);

        // bytes
        uint64_t bytes = sectors * 512ULL;
        tp = 0; tv = bytes; if (tv == 0) tmp[tp++] = '0';
        while (tv) { tmp[tp++] = '0' + (tv % 10); tv /= 10; }
        for (int i = 0; i < tp; ++i) buf[i] = tmp[tp-1-i]; buf[tp] = '\0';
        klog("size (bytes) /1000000 for MB's:"); klog(buf);   

        klog("FS superblock total_blocks:");
        klogf("%llu", (unsigned long long)g_superblock.total_blocks);
    }

}
