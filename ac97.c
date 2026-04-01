#include "ac97.h"
#include "pci.h"
#include "klog.h"
#include "io.h"
#include "fs.h"
#include "irq.h"
#include <stdint.h>

typedef struct { uint32_t addr; uint32_t len; } ac97_bdl_entry_t;

/* controller location saved for later use by playback routines */
static int ac_bus = 0, ac_slot = 0, ac_func = 0;
/* saved NAM/NABM info */
static int nam_is_io = 0;
static uint32_t nam_base = 0;
static int nabm_is_io = 0;
static uint32_t nabm_base = 0;

int ac97_init(void) {
    uint8_t bus = 0, slot = 0, func = 0;
    if (!pci_find_class(0x04, 0x01, &bus, &slot, &func)) {
        kklog("AC97: no audio controller (class 0x04/0x01) found");
        return 0;
    }
    klogf("AC97: controller at %d:%d.%d", bus, slot, func);

    /* Enable I/O space and Bus Mastering in PCI command register */
    uint32_t cmdcfg = pci_read_config32(bus, slot, func, 0x04);
    uint32_t newcmd = cmdcfg | 0x00000005U; /* bit0 = I/O space, bit2 = bus master */
    if (newcmd != cmdcfg) {
        pci_write_config32(bus, slot, func, 0x04, newcmd);
        klogf("AC97: PCI command updated 0x%x -> 0x%x", cmdcfg, newcmd);
    }

    uint32_t bar0 = pci_get_bar0(bus, slot, func);
    uint32_t bar1 = pci_read_config32(bus, slot, func, 0x14);
    klogf("AC97: BAR0=0x%x BAR1=0x%x", bar0, bar1);

    ac_bus = bus; ac_slot = slot; ac_func = func;
    nam_is_io = (bar0 & 0x1) ? 1 : 0;
    nam_base = nam_is_io ? (bar0 & ~0x3U) : (bar0 & ~0xFU);
    nabm_is_io = (bar1 & 0x1) ? 1 : 0;
    nabm_base = nabm_is_io ? (bar1 & ~0x3U) : (bar1 & ~0xFU);

    klogf("AC97: NAM base=0x%x (%s) NABM base=0x%x (%s)", nam_base, nam_is_io?"io":"mmio", nabm_base, nabm_is_io?"io":"mmio");

    /* helpers for NAM (word) and NABM (byte/word/dword) access */
    #define NAM_READ16(off) ( (nam_is_io) ? inw((uint16_t)(nam_base + (off))) : *((volatile uint16_t*)(uintptr_t)(nam_base + (off))) )
    #define NAM_WRITE16(off,val) do { if (nam_is_io) outw((uint16_t)(nam_base + (off)), (uint16_t)(val)); else *((volatile uint16_t*)(uintptr_t)(nam_base + (off))) = (uint16_t)(val); } while(0)
    #define NABM_READ32(off) ( (nabm_is_io) ? inl((uint16_t)(nabm_base + (off))) : *((volatile uint32_t*)(uintptr_t)(nabm_base + (off))) )
    #define NABM_WRITE32(off,val) do { if (nabm_is_io) outl((uint16_t)(nabm_base + (off)), (uint32_t)(val)); else *((volatile uint32_t*)(uintptr_t)(nabm_base + (off))) = (uint32_t)(val); } while(0)
    #define NABM_READ8(off) ( (nabm_is_io) ? inb((uint16_t)(nabm_base + (off))) : *((volatile uint8_t*)(uintptr_t)(nabm_base + (off))) )
    #define NABM_WRITE8(off,val) do { if (nabm_is_io) outb((uint16_t)(nabm_base + (off)), (uint8_t)(val)); else *((volatile uint8_t*)(uintptr_t)(nabm_base + (off))) = (uint8_t)(val); } while(0)

    /* Resume from cold reset: write 0x2 to Global Control (NABM 0x2C). If you want interrupts enabled, use 0x3. */
    NABM_WRITE32(0x2C, 0x2);
    /* small delay to let controller process the resume */
    for (volatile int i = 0; i < 100000; ++i) { asm volatile ("nop"); }

    /* Reset NAM registers by writing any value to NAM 0x00 (word). Then read capabilities. */
    NAM_WRITE16(0x00, 0x0000);
    uint16_t nam_caps = NAM_READ16(0x00);
    klogf("AC97: NAM capabilities=0x%x", nam_caps);

    /* Set PCM output volume to maximum (0) to ensure audio is audible */
    NAM_WRITE16(0x18, 0x0000);

    kklog("AC97: controller init + codec reset complete (DMA/IRQ not configured yet)");
    return 1;
}

/* Playback support (simple WAV loader + start). This implementation is limited (16-bit PCM stereo, small files)
   and uses the static ac97_dma_buffer and ac97_bdl in ac97_data.c
*/

/* forward externs to the data in ac97_data.c */
extern uint8_t ac97_dma_buffer[];
extern void* ac97_bdl;

static int ac97_irq_registered = 0;

static void ac97_stop_internal(uint8_t nabm_is_io_local, uint32_t nabm_base_local) {
    /* stop stream (stream control at 0x1B write 0 to stop) */
    if (nabm_is_io_local) outb((uint16_t)(nabm_base_local + 0x1B), 0x0);
    else *((volatile uint8_t*)(uintptr_t)(nabm_base_local + 0x1B)) = 0x0;
}

static void ac97_irq_handler(int irq) {
    kklogf("AC97 IRQ: %d", irq);
    /* for now just stop the stream on IRQ (end-of-transfer) */
    /* We need local access to nabm_base/is_io but keep it simple: write global NABM stop if possible by re-reading PCI BAR */
    uint8_t bus=0, slot=0, func=0; /* re-find controller */
    if (!pci_find_class(0x04, 0x01, &bus, &slot, &func)) return;
    uint32_t bar1 = pci_read_config32(bus, slot, func, 0x14);
    int nabm_is_io_local = (bar1 & 0x1) ? 1 : 0;
    uint32_t nabm_base_local = nabm_is_io_local ? (bar1 & ~0x3U) : (bar1 & ~0xFU);
    ac97_stop_internal(nabm_is_io_local, nabm_base_local);
}

int ac97_play_wav(const char* filename) {
    inode_t root;
    read_inode(0, &root);
    int inode_num = dir_lookup(&root, filename);
    if (inode_num < 0) {
        kklog("ac97: file not found");
        return 0;
    }
    inode_t file_node;
    read_inode(inode_num, &file_node);
    uint8_t hdr[44];
    if (fs_read(inode_num, &file_node, 0, 44, hdr) < 44) {
        kklog("ac97: invalid wav header");
        return 0;
    }
    if (!(hdr[0]=='R' && hdr[1]=='I' && hdr[2]=='F' && hdr[3]=='F' && hdr[8]=='W' && hdr[9]=='A' && hdr[10]=='V' && hdr[11]=='E')) {
        kklog("ac97: not a WAV file");
        return 0;
    }
    uint16_t audio_format = hdr[20] | (hdr[21] << 8);
    uint16_t channels = hdr[22] | (hdr[23] << 8);
    uint32_t sample_rate = hdr[24] | (hdr[25] << 8) | (hdr[26] << 16) | (hdr[27] << 24);
    uint16_t bits_per_sample = hdr[34] | (hdr[35] << 8);
    uint32_t data_size = hdr[40] | (hdr[41] << 8) | (hdr[42] << 16) | (hdr[43] << 24);
    if (audio_format != 1 || bits_per_sample != 16 || channels != 2) {
        kklog("ac97: unsupported WAV format (need 16-bit PCM stereo)");
        return 0;
    }
    if (data_size == 0) {
        kklog("ac97: empty WAV data");
        return 0;
    }
    /* limit to buffer size */
    if (data_size > 65536) data_size = 65536;
    int got = fs_read(inode_num, &file_node, 44, data_size, ac97_dma_buffer);
    if (got <= 0) {
        kklog("ac97: failed to read wav data");
        return 0;
    }

    /* build BDL entries (simple single chunk or split into chunks <= 0x10000) */
    ac97_bdl_entry_t* bdl = (ac97_bdl_entry_t*)ac97_bdl;
    uint32_t remaining = (uint32_t)got;
    uint32_t offset = 0;
    int idx = 0;
    while (remaining > 0 && idx < 32) {
        uint32_t chunk = remaining;
        if (chunk > 0x10000) chunk = 0x10000;
        bdl[idx].addr = (uint32_t)(uintptr_t)(ac97_dma_buffer + offset);
        bdl[idx].len = chunk;
        remaining -= chunk;
        offset += chunk;
        idx++;
    }
    if (idx == 0) return 0;

    /* write BDL base */
    uint32_t bar1 = pci_read_config32(ac_bus, ac_slot, ac_func, 0x14);
    int nabm_is_io_local = (bar1 & 0x1) ? 1 : 0;
    uint32_t nabm_base_local = nabm_is_io_local ? (bar1 & ~0x3U) : (bar1 & ~0xFU);
    if (nabm_is_io_local) {
        outl((uint16_t)(nabm_base_local + 0x10), (uint32_t)(uintptr_t)bdl);
        outb((uint16_t)(nabm_base_local + 0x15), (uint8_t)(idx - 1));
        /* reset stream */
        outb((uint16_t)(nabm_base_local + 0x1B), 0x2);
        for (volatile int i = 0; i < 10000; ++i) asm volatile ("nop");
        /* start */
        outb((uint16_t)(nabm_base_local + 0x1B), 0x1);
        /* enable global interrupts */
        outl((uint16_t)(nabm_base_local + 0x2C), 0x3);
    } else {
        *((volatile uint32_t*)(uintptr_t)(nabm_base_local + 0x10)) = (uint32_t)(uintptr_t)bdl;
        *((volatile uint8_t*)(uintptr_t)(nabm_base_local + 0x15)) = (uint8_t)(idx - 1);
        *((volatile uint8_t*)(uintptr_t)(nabm_base_local + 0x1B)) = 0x2;
        for (volatile int i = 0; i < 10000; ++i) asm volatile ("nop");
        *((volatile uint8_t*)(uintptr_t)(nabm_base_local + 0x1B)) = 0x1;
        *((volatile uint32_t*)(uintptr_t)(nabm_base_local + 0x2C)) = 0x3;
    }

    /* register IRQ handler */
    uint32_t intr = pci_read_config32(ac_bus, ac_slot, ac_func, 0x3C);
    uint8_t irq_line = intr & 0xFF;
    irq_install_handler((int)irq_line, ac97_irq_handler);
    ac97_irq_registered = 1;

    kklogf("ac97: playing %s (rate=%d channels=%d bytes=%d) IRQ=%d", filename, sample_rate, channels, got, irq_line);
    return 1;
}

void ac97_stop(void) {
    /* uninstall handlers and stop playback */
    uint32_t intr = pci_read_config32(0,0,0,0x00); /* dummy try - best effort */
    if (ac97_irq_registered) {
        /* try to find controller and its irq */
        uint8_t bus=0, slot=0, func=0;
        if (pci_find_class(0x04, 0x01, &bus, &slot, &func)) {
            uint32_t intr2 = pci_read_config32(bus, slot, func, 0x3C);
            uint8_t irq_line = intr2 & 0xFF;
            irq_uninstall_handler((int)irq_line);
        }
        ac97_irq_registered = 0;
    }
}

int ac97_play_tone(int freq_hz, int ms) {
    /* Generate a short sine tone into ac97_dma_buffer (16-bit stereo) */
    const uint32_t sample_rate = 48000;
    uint32_t frames = (sample_rate * (uint32_t)ms) / 1000;
    if (frames == 0) return 0;
    if (frames > 16384) frames = 16384; /* keep under 64KiB (frames*4 bytes) */
    /* simple square wave generator (no math lib) */
    uint32_t period = sample_rate / (uint32_t)freq_hz;
    if (period < 2) period = 2;
    int16_t amp = 12000;
    for (uint32_t i = 0; i < frames; i++) {   
        int16_t sample = ((i % period) < (period / 2)) ? amp : -amp;
        uint32_t idx = i * 4;
        ac97_dma_buffer[idx + 0] = (uint8_t)(sample & 0xFF);
        ac97_dma_buffer[idx + 1] = (uint8_t)((sample >> 8) & 0xFF);
        ac97_dma_buffer[idx + 2] = (uint8_t)(sample & 0xFF);
        ac97_dma_buffer[idx + 3] = (uint8_t)((sample >> 8) & 0xFF);
    }

    /* build single BDL entry */
    ac97_bdl_entry_t* bdl = (ac97_bdl_entry_t*)ac97_bdl;
    bdl[0].addr = (uint32_t)(uintptr_t)ac97_dma_buffer;
    bdl[0].len = frames * 4;

    uint32_t bar1 = pci_read_config32(ac_bus, ac_slot, ac_func, 0x14);
    int nabm_is_io_local = (bar1 & 0x1) ? 1 : 0;
    uint32_t nabm_base_local = nabm_is_io_local ? (bar1 & ~0x3U) : (bar1 & ~0xFU);
    if (nabm_is_io_local) {
        outl((uint16_t)(nabm_base_local + 0x10), (uint32_t)(uintptr_t)bdl);
        outb((uint16_t)(nabm_base_local + 0x15), 0);
        outb((uint16_t)(nabm_base_local + 0x1B), 0x2);
        for (volatile int i = 0; i < 10000; ++i) asm volatile ("nop");
        outb((uint16_t)(nabm_base_local + 0x1B), 0x1);
        outl((uint16_t)(nabm_base_local + 0x2C), 0x3);
    } else {
        *((volatile uint32_t*)(uintptr_t)(nabm_base_local + 0x10)) = (uint32_t)(uintptr_t)bdl;
        *((volatile uint8_t*)(uintptr_t)(nabm_base_local + 0x15)) = 0;
        *((volatile uint8_t*)(uintptr_t)(nabm_base_local + 0x1B)) = 0x2;
        for (volatile int i = 0; i < 10000; ++i) asm volatile ("nop");
        *((volatile uint8_t*)(uintptr_t)(nabm_base_local + 0x1B)) = 0x1;
        *((volatile uint32_t*)(uintptr_t)(nabm_base_local + 0x2C)) = 0x3;
    }

    /* don't register IRQ for tone test; user can still use IRQ path if needed */
    return 1;
}
