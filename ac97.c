#include "ac97.h"
#include "pci.h"
#include "klog.h"
#include "io.h"
#include <stdint.h>
extern uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
extern void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
static pci_device_t g_dev;

struct ac97_bdl_entry {
    uint32_t buffer_addr;
    uint16_t length;
    uint16_t flags;
} __attribute__((packed));

static struct ac97_bdl_entry bdl[1] __attribute__((aligned(8)));
static int16_t audio_buffer[32000]; 

int ac97_init(void) {
    kklog("ac97: init start\n");
    if (pci_find_class(0x04, 0x01, &g_dev, 0) != 0) {
        kklog("ac97: no AC'97 device found\n");
        return -1;
    }
    kklogf("ac97: vendor=0x%x dev=0x%x BAR0=0x%x BAR1=0x%x\n",
           g_dev.vendor_id, g_dev.device_id, g_dev.bar0, g_dev.bar1);
    return 0;
}

int ac97_play_test_tone(void) {
    if (g_dev.vendor_id == 0) {
        kklog("ac97: No device!\n");
        return -1;
    }
    uint32_t cmd = pci_config_read(g_dev.bus, g_dev.device, g_dev.function, 0x04);
    pci_config_write(g_dev.bus, g_dev.device, g_dev.function, 0x04, cmd | 0x05);
    uint32_t real_bar0 = pci_config_read(g_dev.bus, g_dev.device, g_dev.function, 0x10);
    uint32_t real_bar1 = pci_config_read(g_dev.bus, g_dev.device, g_dev.function, 0x14);
    uint16_t nam_port = real_bar0 & ~0x3;  
    uint16_t nabm_port = real_bar1 & ~0x3; 

    if (nam_port == 0 || nabm_port == 0) {
        kklog("ac97: PORTY JSOU STALE NULA!\n");
        return -1;
    }
    kklog("ac97: Setting up mixer...\n");
    outw(nam_port + 0x00, 1);
    outw(nam_port + 0x02, 0x0000); 
    outw(nam_port + 0x18, 0x0000);
    kklog("ac97: Generating wave...\n");
    int period = 54; 
    int16_t volume = 2000; 
    for (int i = 0; i < 32000; i += 2) {
        if ((i / 2) % period < (period / 2)) {
            audio_buffer[i] = volume; audio_buffer[i+1] = volume;
        } else {
            audio_buffer[i] = -volume; audio_buffer[i+1] = -volume;
        }
    }
    kklog("ac97: Setting up DMA...\n");
    bdl[0].buffer_addr = (uint32_t)&audio_buffer;
    bdl[0].length = 32000;
    bdl[0].flags = 0x8000;
    outb(nabm_port + 0x1B, 0x00);              
    outl(nabm_port + 0x10, (uint32_t)&bdl); 
    outb(nabm_port + 0x15, 0);                
    outb(nabm_port + 0x1B, 0x01);              
    kklog("ac97: BEEEP!\n");
    return 0;
}