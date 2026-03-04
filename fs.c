#include "klog.h"
#include "fs.h"
#include <stdint.h>
#include <string.h>
#include "string.h"

#define ATA_PRIMARY   0x1F0
#define ATA_SECONDARY 0x170
#define BLOCK_SIZE 512
#define ATA_REG_DATA     0
#define ATA_REG_SECCOUNT 2
#define ATA_REG_LBA0     3
#define ATA_REG_LBA1     4
#define ATA_REG_LBA2     5
#define ATA_REG_DRIVE    6
#define ATA_REG_STATUS   7
#define ATA_REG_CMD      7
#define ATA_CMD_READ  0x20
#define ATA_CMD_WRITE 0x30
#define SECTOR_SIZE 512
#define INODE_TABLE_START 3
#define INODE_SIZE 64
#define INODES_PER_BLOCK (512 / INODE_SIZE)
#define SUPERBLOCK_LBA 0
#define BLOCK_BITMAP_LBA 1
#define INODE_BITMAP_LBA 2
#define INODE_TABLE_LBA 3
#define ROOT_INODE 0

uint16_t current_ata_base = ATA_PRIMARY;

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
    while (inb(current_ata_base + ATA_REG_STATUS) & 0x80);
}

void ata_wait_drq() {
    while (!(inb(current_ata_base + ATA_REG_STATUS) & 0x08));
}

void block_read(uint32_t lba, uint8_t* buf) {
    ata_wait_busy();
    outb(current_ata_base + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(current_ata_base + ATA_REG_SECCOUNT, 1);
    outb(current_ata_base + ATA_REG_LBA0, lba & 0xFF);
    outb(current_ata_base + ATA_REG_LBA1, (lba >> 8) & 0xFF);
    outb(current_ata_base + ATA_REG_LBA2, (lba >> 16) & 0xFF);
    outb(current_ata_base + ATA_REG_CMD, ATA_CMD_READ);
    ata_wait_drq();
    insw(current_ata_base + ATA_REG_DATA, buf, SECTOR_SIZE / 2);
}

void block_write(uint32_t lba, const uint8_t* buf) {
    ata_wait_busy();
    outb(current_ata_base + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(current_ata_base + ATA_REG_SECCOUNT, 1);
    outb(current_ata_base + ATA_REG_LBA0, lba & 0xFF);
    outb(current_ata_base + ATA_REG_LBA1, (lba >> 8) & 0xFF);
    outb(current_ata_base + ATA_REG_LBA2, (lba >> 16) & 0xFF);
    outb(current_ata_base + ATA_REG_CMD, ATA_CMD_WRITE);
    ata_wait_drq();
    outsw(current_ata_base + ATA_REG_DATA, buf, SECTOR_SIZE / 2);
}

superblock_t g_superblock;
static uint32_t inode_table_blocks = 0;
static uint32_t block_bitmap_sectors = 0;
static uint32_t inode_bitmap_sectors = 0;
static uint32_t block_bitmap_bytes = 0;

static int get_block_bitmap_bit(uint32_t idx) {
    if (idx >= g_superblock.total_blocks) return 1;
    uint32_t bits_per_sector = SECTOR_SIZE * 8;
    uint32_t sector_idx = idx / bits_per_sector;
    uint32_t within = idx % bits_per_sector; 
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

int alloc_block() {
    for (uint32_t i = 0; i < g_superblock.total_blocks; i++) {
        if (!get_block_bitmap_bit(i)) {
            set_block_bitmap_bit(i);
            return (int)i;
        }
    }
    return -1;
}

static void clear_block_bitmap_bit(uint32_t idx) {
    if (idx >= g_superblock.total_blocks) return;
    uint32_t bits_per_sector = SECTOR_SIZE * 8;
    uint32_t sector_idx = idx / bits_per_sector;
    uint32_t within = idx % bits_per_sector;
    uint32_t byte_off = within / 8;
    uint8_t buf[SECTOR_SIZE];
    block_read(g_superblock.bitmap_start + sector_idx, buf);
    buf[byte_off] &= ~(1 << (within % 8));
    block_write(g_superblock.bitmap_start + sector_idx, buf);
}

void free_block(uint32_t idx) {
    clear_block_bitmap_bit(idx);
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
        for (uint32_t j = 0; j < SECTOR_SIZE; ++j) tmp[j] = 0;
        if (tocopy > 0) memcpy(tmp, src, tocopy);
        block_write(g_superblock.bitmap_start + i, tmp);
        src += tocopy;
        if (bytes_left > SECTOR_SIZE) bytes_left -= SECTOR_SIZE; else bytes_left = 0;
    }
}

void select_drive(uint16_t base, uint8_t slave) {
    current_ata_base = base;
    outb(base + ATA_REG_DRIVE, slave ? 0xB0 : 0xA0);
    for(int i=0; i<4; i++) inb(base + ATA_REG_STATUS);
}

uint32_t ata_get_total_sectors() {
    uint16_t id[256];
    outb(current_ata_base + ATA_REG_DRIVE, 0xA0);
    outb(current_ata_base + ATA_REG_SECCOUNT, 0);
    outb(current_ata_base + ATA_REG_LBA0, 0);
    outb(current_ata_base + ATA_REG_LBA1, 0);
    outb(current_ata_base + ATA_REG_LBA2, 0);
    outb(current_ata_base + ATA_REG_CMD, 0xEC);
    ata_wait_busy();
    if (inb(current_ata_base + ATA_REG_STATUS) & 0x01) return 0;
    ata_wait_drq();
    insw(current_ata_base + ATA_REG_DATA, id, 256);
    return ((uint32_t)id[61] << 16) | id[60];
}

void init_fs() {
    current_ata_base = ATA_PRIMARY;
    uint32_t drive_sectors = ata_get_total_sectors();
    if (drive_sectors == 0) {
        klog("Primary Master not found, trying Secondary...");
        current_ata_base = ATA_SECONDARY;
        drive_sectors = ata_get_total_sectors();
    }
    if (drive_sectors == 0) {
        klog("ERROR: No IDE drive found!");
        return;
    }
    superblock_t sb;
    uint8_t buf[SECTOR_SIZE];
    int i;
    uint64_t total_blocks = drive_sectors;
    block_bitmap_bytes = (total_blocks + 7) / 8;
    if (block_bitmap_bytes > BLOCK_BITMAP_MAX_SIZE) block_bitmap_bytes = BLOCK_BITMAP_MAX_SIZE;
    block_bitmap_sectors = (block_bitmap_bytes + SECTOR_SIZE - 1) / SECTOR_SIZE;
    inode_bitmap_sectors = (INODE_BITMAP_SIZE + SECTOR_SIZE - 1) / SECTOR_SIZE;
    sb.magic = 0x5A4C534A;
    sb.block_size = BLOCK_SIZE;
    sb.total_blocks = total_blocks;
    sb.inode_count = INODE_BITMAP_SIZE * 8;
    sb.bitmap_start = 1;
    sb.inode_start = sb.bitmap_start + block_bitmap_sectors + inode_bitmap_sectors;
    inode_table_blocks = (sb.inode_count + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK;
    sb.data_start = sb.inode_start + inode_table_blocks;
    block_read(0, buf);
    for (i = 0; i < (int)sizeof(sb); ++i)
        ((uint8_t*)&g_superblock)[i] = buf[i];

    if (g_superblock.magic != 0x5A4C534A) {
        for (i = 0; i < SECTOR_SIZE; ++i) buf[i] = 0;
        for (i = 0; i < (int)sizeof(sb); ++i) buf[i] = ((uint8_t*)&sb)[i];
        block_write(0, buf);
        uint8_t zbuf[SECTOR_SIZE];
        for (i = 0; i < SECTOR_SIZE; ++i) zbuf[i] = 0;
        for (uint32_t s = 0; s < block_bitmap_sectors; ++s)
            block_write(sb.bitmap_start + s, zbuf);
        memset(inode_bitmap, 0, INODE_BITMAP_SIZE);
        g_superblock = sb;
        for (i = 0; i < (int)sb.data_start; i++)
            set_block_bitmap_bit((uint32_t)i);
        inode_bitmap[0] |= 1;
        save_inode_bitmap();
        create_root();
    } else {
        block_bitmap_bytes = (g_superblock.total_blocks + 7) / 8;
        if (block_bitmap_bytes > BLOCK_BITMAP_MAX_SIZE) block_bitmap_bytes = BLOCK_BITMAP_MAX_SIZE;
        block_bitmap_sectors = (block_bitmap_bytes + SECTOR_SIZE - 1) / SECTOR_SIZE;
        inode_bitmap_sectors = (INODE_BITMAP_SIZE + SECTOR_SIZE - 1) / SECTOR_SIZE;
        inode_table_blocks = (g_superblock.inode_count + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK;
        load_block_bitmap();
        load_inode_bitmap();
    }
    if (g_superblock.magic != 0x5A4C534A) g_superblock = sb;
    if (drive_sectors == 0) {
        inode_t root; read_inode(0, &root);
        int idx = dir_lookup(&root, ".disksize");
        if (idx >= 0) {
            uint8_t tmp[64];
            inode_t file_node;
            read_inode((uint32_t)idx, &file_node);
            int r = fs_read((uint32_t)idx, &file_node, 0, sizeof(tmp)-1, tmp);
            if (r > 0) {
                tmp[r] = '\0';
                uint32_t v = 0;
                for (int ii = 0; ii < r && tmp[ii]; ++ii) {
                    if (tmp[ii] < '0' || tmp[ii] > '9') break;
                    v = v * 10 + (tmp[ii] - '0');
                }
                if (v > 0) {
                    g_superblock.total_blocks = v;
                    block_bitmap_bytes = (g_superblock.total_blocks + 7) / 8;
                    block_bitmap_sectors = (block_bitmap_bytes + SECTOR_SIZE - 1) / SECTOR_SIZE;
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

void free_inode(int idx) {
    if (idx < 0 || (uint32_t)idx >= g_superblock.inode_count) return;
    inode_bitmap[idx / 8] &= ~(1 << (idx % 8));
    save_inode_bitmap();
}

struct dirent {
    uint32_t inode;
    char name[28];
};

int dir_lookup(inode_t* dir, const char* name) {
    if (dir->type != 2) return -1;
    
    uint8_t buf[SECTOR_SIZE];
    int entries_per_block = SECTOR_SIZE / sizeof(struct dirent);

    for (int b = 0; b < 12; b++) {
        uint32_t lba = dir->direct[b];
        if (lba == 0) break; 

        block_read(lba, buf);
        struct dirent* entries = (struct dirent*)buf;

        for (int i = 0; i < entries_per_block; i++) {
            if (entries[i].inode == 0) continue;
            if (strcmp(entries[i].name, name) == 0) {
                return (int)entries[i].inode;
            }
        }
    }
    
    return -1;
}

int dir_add(uint32_t dir_inode_id, inode_t* dir, const char* name, uint32_t inode_idx) {
    uint8_t buf[SECTOR_SIZE];
    for (int b = 0; b < 12; b++) {
        if (dir->direct[b] == 0) {
            uint32_t new_block = alloc_block();
            if (new_block == 0) return -1; 
            dir->direct[b] = new_block;
            memset(buf, 0, SECTOR_SIZE);
            write_inode(dir_inode_id, dir);
        } else {
            block_read(dir->direct[b], buf);
        }

        struct dirent* entries = (struct dirent*)buf;
        for (int i = 0; i < (SECTOR_SIZE / sizeof(struct dirent)); i++) {
            if (entries[i].inode == 0) { 
                entries[i].inode = inode_idx;
                strncpy(entries[i].name, name, 31);
                block_write(dir->direct[b], buf);
                return 0; 
            }
        }
    
    }
    klog("ERROR: FULL, MAKE AN INDIRECT BLOCK SUPPORT");
    return -1;
}

int dir_remove(inode_t* dir, const char* name) {
    if (dir->type != 2) return -1;
    uint8_t buf[SECTOR_SIZE];
    block_read((uint32_t)dir->direct[0], buf);
    struct dirent* entries = (struct dirent*)buf;
    int entry_count = SECTOR_SIZE / sizeof(struct dirent);

    for (int i = 0; i < entry_count; i++) {
        if (entries[i].inode == 0) continue;
        if (strcmp(entries[i].name, name) == 0) {
            entries[i].inode = 0;
            for (int j = 0; j < (int)sizeof(entries[i].name); ++j) entries[i].name[j] = '\0';
            block_write((uint32_t)dir->direct[0], buf);
            return 0;
        }
    }
    return -1;
}

uint32_t fs_create_file(const char* name, const char* main_tag) {
    if (!main_tag || strlen(main_tag) == 0) return (uint32_t)-1;
    int idx = alloc_inode();
    if (idx < 0) return (uint32_t)-1;
    inode_t node;
    memset(&node, 0, sizeof(node));
    node.type = 1;
    node.size = 0;
    strncpy(node.main_tag, main_tag, TAG_LEN);
    strncpy(node.tags[0], "normal", TAG_LEN);

    int b = alloc_block();
    if (b < 0) {
        free_inode(idx);
        return (uint32_t)-1;
    }

    node.direct[0] = b;
    write_inode(idx, &node);
    inode_t root;
    read_inode(0, &root);
    dir_add(0, &root, name, (uint32_t)idx);
    return (uint32_t)idx;
}

int fs_find_by_tag(const char* tag, uint32_t* results, int max_results) {
    int found_count = 0;
    inode_t temp_node;
    for (uint32_t i = 1; i < g_superblock.inode_count; i++) {
        if (!(inode_bitmap[i / 8] & (1 << (i % 8)))) continue;
        read_inode(i, &temp_node);
        int match = (strcmp(temp_node.main_tag, tag) == 0);
        for (int t = 0; t < MAX_TAGS && !match; t++) {
            if (strcmp(temp_node.tags[t], tag) == 0) match = 1;
        }
        if (match) {
            results[found_count++] = i;
            if (found_count >= max_results) break;
        }
    }
    return found_count;
}

int fs_write(uint32_t inode_idx, const uint8_t* data, size_t len) {
    inode_t node;
    read_inode(inode_idx, &node);
    size_t written = 0;
    uint8_t buf[SECTOR_SIZE];
    while (written < len) {
        uint32_t block_index = written / SECTOR_SIZE;
        uint32_t offset_in_block = written % SECTOR_SIZE;
        if (block_index >= 12) {
            break; 
        }
        if (node.direct[block_index] == 0) {
            uint32_t new_block = alloc_block();
            if (new_block == 0) {
                return -1;
            }
            node.direct[block_index] = new_block;
            write_inode(inode_idx, &node);
            memset(buf, 0, SECTOR_SIZE);
        } else {
            block_read(node.direct[block_index], buf);
        }
        size_t space_in_sector = SECTOR_SIZE - offset_in_block;
        size_t chunk_size = len - written;
        if (chunk_size > space_in_sector) {
            chunk_size = space_in_sector;
        }
        memcpy(buf + offset_in_block, data + written, chunk_size);
        block_write(node.direct[block_index], buf);

        written += chunk_size;
    }
    if (written > node.size) {
        node.size = (uint32_t)written;
        write_inode(inode_idx, &node);
    }
    return (int)written;
}

int fs_read(uint32_t inode_idx, inode_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    uint32_t bytes_read = 0;
    uint8_t sector_buf[512];
    if (offset + size > node->size) {
        size = node->size - offset;
    }
    while (bytes_read < size) {
        uint32_t current_global_offset = offset + bytes_read;
        uint32_t block_index = current_global_offset / 512;
        uint32_t offset_in_block = current_global_offset % 512;
        if (block_index >= 12) break;
        uint32_t block_addr = node->direct[block_index];
        if (block_addr == 0) {
            memset(sector_buf, 0, 512);
        } else {
            block_read(block_addr, sector_buf);
        }
        uint32_t chunk_size = 512 - offset_in_block;
        if (chunk_size > (size - bytes_read)) {
            chunk_size = size - bytes_read;
        }
        memcpy(buffer + bytes_read, sector_buf + offset_in_block, chunk_size);
        bytes_read += chunk_size;
    }
    return bytes_read;
}

int fs_delete_file(const char* name) {
    inode_t root;
    read_inode(ROOT_INODE, &root);
    int inode_num = dir_lookup(&root, name);
    if (inode_num < 0)
        return -1;
    inode_t node;
    read_inode(inode_num, &node);
    for (int i = 0; i < INODE_DIRECT; i++) {
        if (node.direct[i] != 0) {
            free_block(node.direct[i]);
            node.direct[i] = 0;
        }
    }
    free_inode(inode_num);
    dir_remove(&root, name);
    return 0;
}