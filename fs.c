//NOT ANYMORE FIXED SIZE i hope

#include "fs.h"
#include <stdint.h>
#include <string.h>
#include "string.h"

//
#define BLOCK_SIZE 512
#define ATA_PRIMARY_DATA   0x1F0
#define ATA_PRIMARY_STATUS 0x1F7
#define ATA_PRIMARY_CMD    0x1F7
#define ATA_PRIMARY_SECCOUNT 0x1F2
#define ATA_PRIMARY_LBA0   0x1F3
#define ATA_PRIMARY_LBA1   0x1F4
#define ATA_PRIMARY_LBA2   0x1F5
#define ATA_PRIMARY_DRIVE  0x1F6
//
#define ATA_CMD_READ  0x20
#define ATA_CMD_WRITE 0x30
#define SECTOR_SIZE 512
//
#define INODE_TABLE_START 3
#define INODE_SIZE 64
#define INODES_PER_BLOCK (512 / INODE_SIZE)
//
#define SUPERBLOCK_LBA 0
#define BLOCK_BITMAP_LBA 1
#define INODE_BITMAP_LBA 2
#define INODE_TABLE_LBA 3
// DATA_LBA is computed at runtime once inode table size is known

// Global filesystem layout state (filled during init_fs)
superblock_t g_superblock;
static uint32_t inode_table_blocks = 0;
static uint32_t block_bitmap_sectors = 0;
static uint32_t inode_bitmap_sectors = 0;
// effective block bitmap size in bytes (computed at init)
static uint32_t block_bitmap_bytes = 0;

// helpers for disk-backed block bitmap (bits per sector = SECTOR_SIZE*8 = 4096)
static int get_block_bitmap_bit(uint32_t idx) {
    if (idx >= g_superblock.total_blocks) return 1; // out of range considered used
    uint32_t bits_per_sector = SECTOR_SIZE * 8;
    uint32_t sector_idx = idx / bits_per_sector;
    uint32_t within = idx % bits_per_sector; // 0..4095
    uint32_t byte_off = within / 8;
    uint8_t buf[SECTOR_SIZE];
    block_read(g_superblock.bitmap_start + sector_idx, buf);
    return (buf[byte_off] >> (within % 8)) & 1;
}

static void set_block_bitmap_bit(uint32_t idx) {
    if (idx >= g_superblock.total_blocks) return;
    uint32_t bits_per_sector = SECTOR_SIZE * 8;
    uint32_t sector_idx = idx / bits_per_sector;
    uint32_t within = idx % bits_per_sector;
    uint32_t byte_off = within / 8;
    uint8_t buf[SECTOR_SIZE];
    block_read(g_superblock.bitmap_start + sector_idx, buf);
    buf[byte_off] |= (1 << (within % 8));
    block_write(g_superblock.bitmap_start + sector_idx, buf);
}
//
int alloc_block() {
    for (uint32_t i = 0; i < g_superblock.total_blocks; i++) {
        if (!get_block_bitmap_bit(i)) {
            set_block_bitmap_bit(i);
            return (int)i;
        }
    }
    return -1;
}

void create_root() {
    inode_t root = {0};
    root.type = 2;

    int b = alloc_block();
    root.direct[0] = b;

    write_inode(0, &root);
}


void read_inode(int idx, inode_t* inode) {
    uint8_t buf[512];
    uint32_t block = g_superblock.inode_start + idx / INODES_PER_BLOCK;
    uint32_t offset = (idx % INODES_PER_BLOCK) * INODE_SIZE;

    block_read(block, buf);
    memcpy(inode, buf + offset, sizeof(inode_t));
}

void write_inode(int idx, inode_t* inode) {
    uint8_t buf[512];
    uint32_t block = g_superblock.inode_start + idx / INODES_PER_BLOCK;
    uint32_t offset = (idx % INODES_PER_BLOCK) * INODE_SIZE;

    block_read(block, buf);
    memcpy(buf + offset, inode, sizeof(inode_t));
    block_write(block, buf);
}



// static fallback buffer and public pointer
static uint8_t block_bitmap_static[BLOCK_BITMAP_MAX_SIZE];
uint8_t *block_bitmap = block_bitmap_static;

void load_block_bitmap() {
    uint32_t bytes_left = block_bitmap_bytes;
    uint8_t tmp[SECTOR_SIZE];
    uint8_t *dst = block_bitmap;
    for (uint32_t i = 0; i < block_bitmap_sectors; ++i) {
        block_read(g_superblock.bitmap_start + i, tmp);
        uint32_t tocopy = bytes_left > SECTOR_SIZE ? SECTOR_SIZE : bytes_left;
        if (tocopy > 0) memcpy(dst, tmp, tocopy);
        dst += tocopy;
        if (bytes_left > SECTOR_SIZE) bytes_left -= SECTOR_SIZE; else bytes_left = 0;
    }
}

void save_block_bitmap() {
    uint32_t bytes_left = block_bitmap_bytes;
    uint8_t tmp[SECTOR_SIZE];
    uint8_t *src = block_bitmap;
    for (uint32_t i = 0; i < block_bitmap_sectors; ++i) {
        uint32_t tocopy = bytes_left > SECTOR_SIZE ? SECTOR_SIZE : bytes_left;
        // prepare sector buffer
        for (uint32_t j = 0; j < SECTOR_SIZE; ++j) tmp[j] = 0;
        if (tocopy > 0) memcpy(tmp, src, tocopy);
        block_write(g_superblock.bitmap_start + i, tmp);
        src += tocopy;
        if (bytes_left > SECTOR_SIZE) bytes_left -= SECTOR_SIZE; else bytes_left = 0;
    }
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void insw(uint16_t port, void* addr, int words) {
    asm volatile("rep insw" : "+D"(addr), "+c"(words) : "d"(port) : "memory");
}

static inline void outsw(uint16_t port, const void* addr, int words) {
    asm volatile("rep outsw" : "+S"(addr), "+c"(words) : "d"(port));
}

void ata_wait_busy() {
    while (inb(ATA_PRIMARY_STATUS) & 0x80); // BSY=1
}

void ata_wait_drq() {
    while (!(inb(ATA_PRIMARY_STATUS) & 0x08)); // DRQ=1
}

// Identify drive and return total LBA28 sectors (0 if error)
uint32_t ata_get_total_sectors() {
    uint16_t id[256];

    // select master drive
    outb(ATA_PRIMARY_DRIVE, 0xA0);
    outb(ATA_PRIMARY_SECCOUNT, 0);
    outb(ATA_PRIMARY_LBA0, 0);
    outb(ATA_PRIMARY_LBA1, 0);
    outb(ATA_PRIMARY_LBA2, 0);
    outb(ATA_PRIMARY_CMD, 0xEC); // IDENTIFY

    // wait for BSY clear
    while (inb(ATA_PRIMARY_STATUS) & 0x80);

    // check for error
    if (inb(ATA_PRIMARY_STATUS) & 0x01) return 0;

    // wait for DRQ
    while (!(inb(ATA_PRIMARY_STATUS) & 0x08));

    insw(ATA_PRIMARY_DATA, id, 256);

    uint32_t sectors = ((uint32_t)id[61] << 16) | id[60];
    if (sectors == 0) {
        // maybe LBA48 or reporting failure — return 0 to fallback later
        return 0;
    }
    return sectors;
}

void block_read(uint32_t lba, uint8_t* buf) {
    ata_wait_busy();
    outb(ATA_PRIMARY_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA0, lba & 0xFF);
    outb(ATA_PRIMARY_LBA1, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_LBA2, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_CMD, ATA_CMD_READ);

    ata_wait_drq();
    insw(ATA_PRIMARY_DATA, buf, SECTOR_SIZE / 2);
}

void block_write(uint32_t lba, const uint8_t* buf) {
    ata_wait_busy();
    outb(ATA_PRIMARY_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, 1);
    outb(ATA_PRIMARY_LBA0, lba & 0xFF);
    outb(ATA_PRIMARY_LBA1, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_LBA2, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_CMD, ATA_CMD_WRITE);

    ata_wait_drq();
    outsw(ATA_PRIMARY_DATA, buf, SECTOR_SIZE / 2);
}


void init_fs() {
    superblock_t sb;
    uint8_t buf[SECTOR_SIZE];
    int i;
    //Detects size
    uint32_t drive_sectors = ata_get_total_sectors();
//    if (drive_sectors == 0) {
//    // tries 65536 sectors if identifies error / nothing
//        drive_sectors = 65536;
//    }
    uint32_t max_blocks = BLOCK_BITMAP_MAX_SIZE * 8;
    uint32_t total_blocks = drive_sectors;
    //if (total_blocks > max_blocks) total_blocks = max_blocks;

    // compute effective bitmap bytes and sector counts
    block_bitmap_bytes = (total_blocks + 7) / 8;
    if (block_bitmap_bytes > BLOCK_BITMAP_MAX_SIZE) block_bitmap_bytes = BLOCK_BITMAP_MAX_SIZE;
    block_bitmap_sectors = (block_bitmap_bytes + SECTOR_SIZE - 1) / SECTOR_SIZE;
    inode_bitmap_sectors = (INODE_BITMAP_SIZE + SECTOR_SIZE - 1) / SECTOR_SIZE;

    // layout of table
    sb.magic = 0x5A4C534A;
    sb.block_size = BLOCK_SIZE;
    sb.total_blocks = total_blocks;
    sb.inode_count = INODE_BITMAP_SIZE * 8;
    sb.bitmap_start = 1;
    sb.inode_start = sb.bitmap_start + block_bitmap_sectors + inode_bitmap_sectors;

    inode_table_blocks = (sb.inode_count + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK;
    sb.data_start = sb.inode_start + inode_table_blocks;

    // try read existing superblock first
    block_read(0, buf);
    for (i = 0; i < (int)sizeof(sb); ++i)
        ((uint8_t*)&g_superblock)[i] = buf[i];

    if (g_superblock.magic != 0x5A4C534A) {
        // writes superblock
        for (i = 0; i < SECTOR_SIZE; ++i) buf[i] = 0;
        for (i = 0; i < (int)sizeof(sb); ++i) buf[i] = ((uint8_t*)&sb)[i];
        block_write(0, buf);

        // zero bitmap sectors on disk (use local zero buffer)
        uint8_t zbuf[SECTOR_SIZE];
        for (i = 0; i < SECTOR_SIZE; ++i) zbuf[i] = 0;
        for (uint32_t s = 0; s < block_bitmap_sectors; ++s)
            block_write(sb.bitmap_start + s, zbuf);

        // zero inode bitmap in memory
        memset(inode_bitmap, 0, INODE_BITMAP_SIZE);

        // set global superblock so bitmap helpers use correct base
        g_superblock = sb;

        // reserve metadata blocks (superblock + bitmaps + inode table) + root inode by setting bits on disk
        for (i = 0; i < (int)sb.data_start; i++)
            set_block_bitmap_bit((uint32_t)i);
        inode_bitmap[0] |= 1;

        // write inode bitmap with root reserved
        save_inode_bitmap();

        create_root();
    } else {
        if (g_superblock.total_blocks > max_blocks) g_superblock.total_blocks = max_blocks;
        block_bitmap_bytes = (g_superblock.total_blocks + 7) / 8;
        if (block_bitmap_bytes > BLOCK_BITMAP_MAX_SIZE) block_bitmap_bytes = BLOCK_BITMAP_MAX_SIZE;
        block_bitmap_sectors = (block_bitmap_bytes + SECTOR_SIZE - 1) / SECTOR_SIZE;
        inode_bitmap_sectors = (INODE_BITMAP_SIZE + SECTOR_SIZE - 1) / SECTOR_SIZE;
        inode_table_blocks = (g_superblock.inode_count + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK;
        load_block_bitmap();
        load_inode_bitmap();
    }
    if (g_superblock.magic != 0x5A4C534A) g_superblock = sb;

    // If IDENTIFY failed to report size, try reading persisted ".disksize" in root
    if (drive_sectors == 0) {
        inode_t root; read_inode(0, &root);
        int idx = dir_lookup(&root, ".disksize");
        if (idx >= 0) {
            uint8_t tmp[64];
            int r = fs_read((uint32_t)idx, tmp, sizeof(tmp)-1);
            if (r > 0) {
                tmp[r] = '\0';
                // parse decimal
                uint32_t v = 0;
                for (int ii = 0; ii < r && tmp[ii]; ++ii) {
                    if (tmp[ii] < '0' || tmp[ii] > '9') break;
                    v = v * 10 + (tmp[ii] - '0');
                }
                if (v > 0) {
                    if (v > max_blocks) v = max_blocks;
                    g_superblock.total_blocks = v;
                    // recompute bitmap bytes/sectors based on persisted value
                    block_bitmap_bytes = (g_superblock.total_blocks + 7) / 8;
                    if (block_bitmap_bytes > BLOCK_BITMAP_MAX_SIZE) block_bitmap_bytes = BLOCK_BITMAP_MAX_SIZE;
                    block_bitmap_sectors = (block_bitmap_bytes + SECTOR_SIZE - 1) / SECTOR_SIZE;
                    // rewrite superblock to reflect applied size
                    for (i = 0; i < SECTOR_SIZE; ++i) buf[i] = 0;
                    for (i = 0; i < (int)sizeof(g_superblock); ++i) buf[i] = ((uint8_t*)&g_superblock)[i];
                    block_write(0, buf);
                }
            }
        }
    }
}
uint8_t inode_bitmap[INODE_BITMAP_SIZE];
void load_inode_bitmap() {
    uint32_t start = g_superblock.bitmap_start + block_bitmap_sectors;
    for (uint32_t i = 0; i < inode_bitmap_sectors; ++i) {
        block_read(start + i, inode_bitmap + i * SECTOR_SIZE);
    }
}
void save_inode_bitmap() {
    uint32_t start = g_superblock.bitmap_start + block_bitmap_sectors;
    for (uint32_t i = 0; i < inode_bitmap_sectors; ++i) {
        block_write(start + i, inode_bitmap + i * SECTOR_SIZE);
    }
}
int alloc_inode() {
    for (uint32_t i = 0; i < g_superblock.inode_count; i++) {
        if (!(inode_bitmap[i / 8] & (1 << (i % 8)))) {
            inode_bitmap[i / 8] |= (1 << (i % 8));
            save_inode_bitmap();
            return (int)i;
        }
    }
    return -1;
}

// dir entry structure
struct dirent {
    uint32_t inode;
    char name[28];
};

int dir_lookup(inode_t* dir, const char* name) {
    if (dir->type != 2) return -1; // not a directory
    
    uint8_t buf[SECTOR_SIZE];
    block_read((uint32_t)dir->direct[0], buf);
    
    struct dirent* entries = (struct dirent*)buf;
    int entry_count = SECTOR_SIZE / sizeof(struct dirent);
    
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].inode == 0) continue;
        if (strcmp(entries[i].name, name) == 0) {
            return entries[i].inode;
        }
    }
    
    return -1;
}

int dir_add(inode_t* dir, const char* name, uint32_t inode) {
    if (dir->type != 2) return -1; // not a directory
    
    uint8_t buf[SECTOR_SIZE];
    block_read((uint32_t)dir->direct[0], buf);
    
    struct dirent* entries = (struct dirent*)buf;
    int entry_count = SECTOR_SIZE / sizeof(struct dirent);
    
    // find empty slot
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].inode == 0) {
            entries[i].inode = inode;
            // simple copy instead of strncpy
            int j = 0;
            while (name[j] && j < 27) {
                entries[i].name[j] = name[j];
                j++;
            }
            entries[i].name[j] = '\0';
            block_write((uint32_t)dir->direct[0], buf);
            return 0;
        }
    }
    
    return -1; // directory full
}

// normal fs files writing
//whole srite/read/delete idk whatever function
//absolute pain
uint32_t fs_create_file(const char* path);
int fs_write(uint32_t inode, const uint8_t* data, size_t len);
int fs_read(uint32_t inode, uint8_t* buf, size_t len);
int fs_delete_file(const char* path);

uint32_t fs_create_file(const char* path) {
    int idx = alloc_inode();
    if (idx < 0) return (uint32_t)-1;
    inode_t node;
    memset(&node, 0, sizeof(node));
    node.type = 1; // regular file
    node.size = 0;

    int b = alloc_block();
    if (b < 0) {
        // rollback inode allocation
        inode_bitmap[idx / 8] &= ~(1 << (idx % 8));
        save_inode_bitmap();
        return (uint32_t)-1;
    }

    node.direct[0] = b;
    write_inode(idx, &node);
    
    // Add file to root directory
    inode_t root;
    read_inode(0, &root);
    dir_add(&root, path, (uint32_t)idx);
    
    return (uint32_t)idx;
}

int fs_write(uint32_t inode, const uint8_t* data, size_t len) {
    inode_t node;
    read_inode(inode, &node);
    if (node.direct[0] == 0) return -1;

    uint8_t buf[SECTOR_SIZE];
    memset(buf, 0, SECTOR_SIZE);
    size_t towrite = len;
    if (towrite > SECTOR_SIZE) towrite = SECTOR_SIZE;
    memcpy(buf, data, towrite);

    block_write((uint32_t)node.direct[0], buf);
    node.size = (uint32_t)towrite;
    write_inode(inode, &node);
    return (int)towrite;
}

int fs_read(uint32_t inode, uint8_t* buf_out, size_t len) {
    inode_t node;
    read_inode(inode, &node);
    if (node.direct[0] == 0) return -1;

    uint8_t buf[SECTOR_SIZE];
    block_read((uint32_t)node.direct[0], buf);
    size_t toread = (len < node.size) ? len : node.size;
    if (toread > SECTOR_SIZE) toread = SECTOR_SIZE;
    memcpy(buf_out, buf, toread);
    return (int)toread;
}

int fs_delete_file(const char* path) {
    //not implemented yet
    //im too lazy to do ths
    return -1;
}
