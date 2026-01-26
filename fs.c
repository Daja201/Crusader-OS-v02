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
#define DATA_LBA (INODE_TABLE_LBA + inode_table_blocks)
//
int alloc_block() {
    for (int i = 0; i < BLOCK_BITMAP_SIZE * 8; i++) {
        if (!(block_bitmap[i / 8] & (1 << (i % 8)))) {
            block_bitmap[i / 8] |= (1 << (i % 8));
            save_block_bitmap();
            return i;
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
    uint32_t block = INODE_TABLE_START + idx / INODES_PER_BLOCK;
    uint32_t offset = (idx % INODES_PER_BLOCK) * INODE_SIZE;

    block_read(block, buf);
    memcpy(inode, buf + offset, sizeof(inode_t));
}

void write_inode(int idx, inode_t* inode) {
    uint8_t buf[512];
    uint32_t block = INODE_TABLE_START + idx / INODES_PER_BLOCK;
    uint32_t offset = (idx % INODES_PER_BLOCK) * INODE_SIZE;

    block_read(block, buf);
    memcpy(buf + offset, inode, sizeof(inode_t));
    block_write(block, buf);
}



uint8_t block_bitmap[BLOCK_BITMAP_SIZE];

void load_block_bitmap() {
    block_read(1, block_bitmap);
}

void save_block_bitmap() {
    block_write(1, block_bitmap);
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

    load_block_bitmap();
    load_inode_bitmap();

    block_read(0, buf);

    for (i = 0; i < (int)sizeof(sb); ++i)
        ((uint8_t*)&sb)[i] = buf[i];

    if (sb.magic != 0x5A4C534A) {
        // ===== FORMAT ===== //
        sb.magic = 0x5A4C534A;
        sb.block_size = BLOCK_SIZE;
        sb.total_blocks = 65536;
        sb.inode_count = 128;
        sb.bitmap_start = 1;
        sb.inode_start = 2;
        sb.data_start = 10;

        for (i = 0; i < SECTOR_SIZE; ++i) buf[i] = 0;
        for (i = 0; i < (int)sizeof(sb); ++i) buf[i] = ((uint8_t*)&sb)[i];
        block_write(0, buf);

        // reset bitmaps
        memset(block_bitmap, 0, BLOCK_BITMAP_SIZE);
        memset(inode_bitmap, 0, INODE_BITMAP_SIZE);

        // reserve metadata blocks
        for (i = 0; i < sb.data_start; i++)
            block_bitmap[i / 8] |= (1 << (i % 8));

        // reserve root inode
        inode_bitmap[0] |= 1;

        save_block_bitmap();
        save_inode_bitmap();

        create_root();
    }

    // ===== NORMAL BOOT ===== //

}

uint8_t inode_bitmap[INODE_BITMAP_SIZE];

void load_inode_bitmap() {
    block_read(2, inode_bitmap);
}


void save_inode_bitmap() {
    block_write(2, inode_bitmap);
}


int alloc_inode() {
    for (int i = 0; i < 128; i++) {
        if (!(inode_bitmap[i / 8] & (1 << (i % 8)))) {
            inode_bitmap[i / 8] |= (1 << (i % 8));
            save_inode_bitmap();
            return i;
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
