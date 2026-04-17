#include "klog.h"
#include "fs.h"
#include <stdint.h>
#include <string.h>
#include "string.h"
#include "io.h"
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
#define INODE_SIZE 128
#define INODES_PER_BLOCK (512 / INODE_SIZE)
#define SUPERBLOCK_LBA 0
#define BLOCK_BITMAP_LBA 1
#define INODE_BITMAP_LBA 2
#define INODE_TABLE_LBA 3
#define ROOT_INODE 0
#define PTRS_PER_BLOCK (SECTOR_SIZE / sizeof(uint32_t))
uint8_t inode_bitmap[INODE_BITMAP_SIZE];
static uint16_t current_ata_base = ATA_PRIMARY;
static uint8_t current_is_slave = 0;
fs_device_t g_drives[MAX_DRIVES];
int g_active_drives = 0;
superblock_t g_superblock;
static uint32_t inode_table_blocks = 0;
static uint32_t block_bitmap_sectors = 0;
static uint32_t inode_bitmap_sectors = 0;
static uint32_t block_bitmap_bytes = 0;
static uint8_t block_bitmap_static[BLOCK_BITMAP_MAX_SIZE];
uint8_t *block_bitmap = block_bitmap_static;
int g_current_drive = 0;

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

int ata_wait_busy(uint16_t base) {
    uint32_t timeout = 1000000;
    while ((inb(base + 7) & 0x80) && --timeout);
    return (timeout == 0) ? -1 : 0;
}

int ata_wait_drq(uint16_t base) {
    uint32_t timeout = 1000000;
    while (!(inb(base + 7) & 0x08) && --timeout);
    return (timeout == 0) ? -1 : 0;
}

void block_read(uint32_t lba, uint8_t* buf) {
    uint8_t drive_bit = current_is_slave ? 0xF0 : 0xE0;
    uint8_t drive_sel = (current_is_slave ? 0xF0 : 0xE0) | ((lba >> 24) & 0x0F);
    outb(current_ata_base + ATA_REG_DRIVE, drive_sel);
    for(int i=0; i<4; i++) inb(current_ata_base + ATA_REG_STATUS);
    ata_wait_busy(current_ata_base);
    outb(current_ata_base + ATA_REG_SECCOUNT, 1);
    outb(current_ata_base + ATA_REG_LBA0, lba & 0xFF);
    outb(current_ata_base + ATA_REG_LBA1, (lba >> 8) & 0xFF);
    outb(current_ata_base + ATA_REG_LBA2, (lba >> 16) & 0xFF);
    outb(current_ata_base + ATA_REG_CMD, ATA_CMD_READ);
    ata_wait_drq(current_ata_base);
    insw(current_ata_base + ATA_REG_DATA, buf, SECTOR_SIZE / 2);
}

void block_write(uint32_t lba, const uint8_t* buf) {
    uint8_t drive_sel = (current_is_slave ? 0xF0 : 0xE0) | ((lba >> 24) & 0x0F);
    outb(current_ata_base + ATA_REG_DRIVE, drive_sel);
    for(int i=0; i<4; i++) inb(current_ata_base + ATA_REG_STATUS);
    ata_wait_busy(current_ata_base);
    outb(current_ata_base + ATA_REG_SECCOUNT, 1);
    outb(current_ata_base + ATA_REG_LBA0, lba & 0xFF);
    outb(current_ata_base + ATA_REG_LBA1, (lba >> 8) & 0xFF);
    outb(current_ata_base + ATA_REG_LBA2, (lba >> 16) & 0xFF);
    outb(current_ata_base + ATA_REG_CMD, ATA_CMD_WRITE);    
    ata_wait_drq(current_ata_base);
    outsw(current_ata_base + ATA_REG_DATA, buf, SECTOR_SIZE / 2);
}


int alloc_block() {
    uint32_t start_idx = 1;
    const uint32_t FS_MAGIC = 0x5A4C534A;
    if (g_superblock.magic == FS_MAGIC && g_superblock.data_start > 1) start_idx = g_superblock.data_start;
    for (uint32_t i = start_idx; i < g_superblock.total_blocks; i++) {
        if (!get_block_bitmap_bit(i)) {
            set_block_bitmap_bit(i);
            return (int)i;
        }
    }
    return 0;
}

void free_block(uint32_t idx) {
    clear_block_bitmap_bit(idx);
}

void create_root() {
    inode_bitmap[0] |= 1;
    save_inode_bitmap();
    inode_t root = {0};
    root.type = 2;
    int b = alloc_block();
    if (b == 0) {
        kklog("create_root: alloc_block failed");
        return;
    }
    root.direct[0] = (uint32_t)b;
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
    current_is_slave = slave;
    outb(base + ATA_REG_DRIVE, 0xA0 | (slave << 4));
    for(int i = 0; i < 4; i++) {
        inb(base + ATA_REG_STATUS);
    }
    while (inb(base + ATA_REG_STATUS) & 0x80); 
}

static uint32_t ata_get_total_sectors_dev(uint16_t base, uint8_t is_slave) {
    uint16_t id[256];
    outb(base + ATA_REG_DRIVE, is_slave ? 0xB0 : 0xA0);
    for(int i=0; i<4; i++) inb(base + ATA_REG_STATUS);
    outb(base + ATA_REG_SECCOUNT, 0);
    outb(base + ATA_REG_LBA0, 0);
    outb(base + ATA_REG_LBA1, 0);
    outb(base + ATA_REG_LBA2, 0);
    outb(base + ATA_REG_CMD, 0xEC);
    uint8_t status = inb(base + ATA_REG_STATUS);
    if (status == 0x00 || status == 0xFF) return 0;
    if (ata_wait_busy(base) < 0) return 0;
    if (inb(base + ATA_REG_STATUS) & 0x01) return 0;
    if (ata_wait_drq(base) < 0) return 0;
    insw(base + ATA_REG_DATA, id, 256);
    return ((uint32_t)id[61] << 16) | id[60];
}

void format_fs() {
    klog_status("FORMATTING STARTED");
    select_drive(g_drives[g_current_drive].ata_base, g_drives[g_current_drive].is_slave);
    g_superblock.magic = 0x5A4C534A;
    g_superblock.block_size = SECTOR_SIZE;
    g_superblock.total_blocks = g_drives[g_current_drive].total_sectors;
    g_superblock.inode_count = g_superblock.total_blocks / 4;
    g_superblock.inode_count = (g_superblock.inode_count + 7) & ~7; 
    if (g_superblock.inode_count == 0) g_superblock.inode_count = 8;
    g_superblock.bitmap_start = BLOCK_BITMAP_LBA;
    block_bitmap_bytes = (uint32_t)((g_superblock.total_blocks + 7) / 8);
    block_bitmap_sectors = (block_bitmap_bytes + SECTOR_SIZE - 1) / SECTOR_SIZE;
    inode_bitmap_sectors = ((g_superblock.inode_count + 7) / 8 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    g_superblock.inode_start = g_superblock.bitmap_start + block_bitmap_sectors + inode_bitmap_sectors;
    inode_table_blocks = (uint32_t)((g_superblock.inode_count * INODE_SIZE + SECTOR_SIZE - 1) / SECTOR_SIZE);
    g_superblock.data_start = g_superblock.inode_start + inode_table_blocks;
    {
        uint8_t sb_buf[SECTOR_SIZE];
        for (int i = 0; i < SECTOR_SIZE; i++) sb_buf[i] = 0;
        memcpy(sb_buf, &g_superblock, sizeof(superblock_t));
        block_write(SUPERBLOCK_LBA, sb_buf);
    }
    for (int i = 0; i < BLOCK_BITMAP_MAX_SIZE; i++) block_bitmap[i] = 0;
    for (int i = 0; i < INODE_BITMAP_SIZE; i++) inode_bitmap[i] = 0;
    save_block_bitmap();
    save_inode_bitmap();
    for (uint32_t i = 0; i < g_superblock.data_start; i++) {
        set_block_bitmap_bit(i);
    }
    create_root();
    klog_status("FORMATTED");
    init_fs();
}

void qformat_fs(void) {
    uint8_t zero_sector[SECTOR_SIZE];
    memset(zero_sector, 0, SECTOR_SIZE);
    klog_status("QUICK FORMATTING STARTED");
    block_write(BLOCK_BITMAP_LBA, zero_sector);
    block_write(INODE_BITMAP_LBA, zero_sector);
    uint32_t num_inode_blocks = 64; 
    for (uint32_t i = 0; i < num_inode_blocks; i++) {
        block_write(INODE_TABLE_LBA + i, zero_sector);
    }
    g_superblock.magic = 0x5A4C534A;
    g_superblock.block_size = SECTOR_SIZE;
    g_superblock.total_blocks = g_drives[g_current_drive].total_sectors;
    g_superblock.inode_count = g_superblock.total_blocks / 4;
    g_superblock.inode_count = (g_superblock.inode_count + 7) & ~7; 
    if (g_superblock.inode_count == 0) g_superblock.inode_count = 8;
    g_superblock.bitmap_start = BLOCK_BITMAP_LBA;
    block_bitmap_bytes = (uint32_t)((g_superblock.total_blocks + 7) / 8);
    block_bitmap_sectors = (block_bitmap_bytes + SECTOR_SIZE - 1) / SECTOR_SIZE;
    inode_bitmap_sectors = ((g_superblock.inode_count + 7) / 8 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    g_superblock.inode_start = g_superblock.bitmap_start + block_bitmap_sectors + inode_bitmap_sectors;
    inode_table_blocks = (uint32_t)((g_superblock.inode_count * INODE_SIZE + SECTOR_SIZE - 1) / SECTOR_SIZE);
    g_superblock.data_start = g_superblock.inode_start + inode_table_blocks;
    uint8_t sb_buf[SECTOR_SIZE];
    memset(sb_buf, 0, SECTOR_SIZE);
    memcpy(sb_buf, &g_superblock, sizeof(superblock_t));
    block_write(SUPERBLOCK_LBA, sb_buf);
    for (int i = 0; i < BLOCK_BITMAP_MAX_SIZE; i++) block_bitmap[i] = 0;
    for (int i = 0; i < INODE_BITMAP_SIZE; i++) inode_bitmap[i] = 0;
    for (uint32_t i = 0; i < g_superblock.data_start; i++) {
        set_block_bitmap_bit(i);
    }
    create_root();
    init_fs();
    klog_status("FORMATTED");
}

void drives() {
    g_active_drives = 0;
    uint16_t ports[] = { 0x1F0, 0x170 };
    for (int p = 0; p < 2; p++) {
        for (int s = 0; s < 2; s++) {
            uint16_t base = ports[p];
            if (inb(base + 7) == 0xFF) continue;
            uint32_t sectors = ata_get_total_sectors_dev(base, s);
            if (sectors > 0 && sectors < 0xFFFFFFF) { 
                if (g_active_drives == 0) {
                    kklogf_green("Disk 0x%x, Slave %d", (uint32_t)base, (uint32_t)s);
                }
                g_drives[g_active_drives].ata_base = base;
                g_drives[g_active_drives].is_slave = s;
                g_drives[g_active_drives].total_sectors = sectors;
                g_active_drives++;
            }
        }
    }
}

void init_fs() {
    if (g_active_drives > 4) g_active_drives = 0;
    if (g_active_drives == 0) {
        kklog("No active drives found");
        return;
    }
    select_drive(g_drives[g_current_drive].ata_base, g_drives[g_current_drive].is_slave);
    {
        uint8_t sb_buf[SECTOR_SIZE];
        block_read(SUPERBLOCK_LBA, sb_buf);
        memcpy(&g_superblock, sb_buf, sizeof(superblock_t));
    }
    const uint32_t FS_MAGIC = 0x5A4C534A;
    if (g_superblock.magic != FS_MAGIC) {
        klog_status("No valid filesystem superblock", 'r');
        return;
    }
    //IT HURTS
    block_bitmap_bytes = (uint32_t)((g_superblock.total_blocks + 7) / 8);
    block_bitmap_sectors = (block_bitmap_bytes + SECTOR_SIZE - 1) / SECTOR_SIZE;
    inode_bitmap_sectors = ((g_superblock.inode_count + 7) / 8 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    inode_table_blocks = (uint32_t)((g_superblock.inode_count * INODE_SIZE + SECTOR_SIZE - 1) / SECTOR_SIZE);
    load_block_bitmap();
    load_inode_bitmap();
    inode_t root;
    read_inode(ROOT_INODE, &root);
    if (root.type != 2) {
        kklog("Creating root directory");
        create_root();
    }
}

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
            if (entries[i].inode >= g_superblock.inode_count) continue;
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
                strncpy(entries[i].name, name, sizeof(entries[i].name) - 1);
                entries[i].name[sizeof(entries[i].name) - 1] = '\0';
                block_write(dir->direct[b], buf);
                return 0; 
            }
        }    
    }
    kklog("ERROR: FULL, MAKE AN INDIRECT BLOCK SUPPORT");
    return -1;
}

int fs_change_drive(int drive_id) {
    if (drive_id < 0 || drive_id >= g_active_drives) {
        return -1;
    }
    g_current_drive = drive_id;
    init_fs(); 
    return 0;
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
        uint32_t phys_block = get_physical_block(inode_idx, &node, block_index, 1);
        if (phys_block == 0) return -1; // volume full
        if (offset_in_block > 0 || (len - written) < SECTOR_SIZE) {
            block_read(phys_block, buf);
        }
        size_t chunk_size = SECTOR_SIZE - offset_in_block;
        if (chunk_size > (len - written)) chunk_size = len - written;
        memcpy(buf + offset_in_block, data + written, chunk_size);
        block_write(phys_block, buf);
        written += chunk_size;
    }
    if (written > node.size) {
        node.size = (uint32_t)written;
        write_inode(inode_idx, &node);
    }
    return (int)written;
}

void free_indirect_tree(uint32_t block_lba, int depth) {
    if (block_lba == 0) return;
    if (depth > 0) {
        uint32_t ptrs[PTRS_PER_BLOCK];
        block_read(block_lba, (uint8_t*)ptrs);
        for (int i = 0; i < PTRS_PER_BLOCK; i++) {
            if (ptrs[i] != 0) {
                free_indirect_tree(ptrs[i], depth - 1);
            }
        }
    }
    free_block(block_lba);
}

int fs_delete_file(const char* name) {
    inode_t root;
    read_inode(ROOT_INODE, &root);    
    int inode_num = dir_lookup(&root, name);
    if (inode_num < 0) {
        return -1;
    }
    inode_t node;
    read_inode(inode_num, &node);
    for (int i = 0; i < INODE_DIRECT; i++) {
        if (node.direct[i] != 0) {
            free_block(node.direct[i]);
            node.direct[i] = 0;
        }
    }
    if (node.single_indirect != 0) {
        free_indirect_tree(node.single_indirect, 1);
        node.single_indirect = 0;
    }
    if (node.double_indirect != 0) {
        free_indirect_tree(node.double_indirect, 2);
        node.double_indirect = 0;
    }
    if (node.triple_indirect != 0) {
        free_indirect_tree(node.triple_indirect, 3);
        node.triple_indirect = 0;
    }
    free_inode(inode_num);
    dir_remove(&root, name);

    return 0;
}

static void zero_block(uint32_t lba) {
    uint8_t zbuf[SECTOR_SIZE] = {0};
    block_write(lba, zbuf);
}

uint32_t get_physical_block(uint32_t inode_idx, inode_t* node, uint32_t logical_idx, int do_alloc) {
    uint32_t ptrs[PTRS_PER_BLOCK];
    if (logical_idx < INODE_DIRECT) {
        if (node->direct[logical_idx] == 0 && do_alloc) {
            node->direct[logical_idx] = alloc_block();
            write_inode(inode_idx, node);
        }
        return node->direct[logical_idx];
    }
    logical_idx -= INODE_DIRECT;

    if (logical_idx < PTRS_PER_BLOCK) {
        if (node->single_indirect == 0 && do_alloc) {
            node->single_indirect = alloc_block();
            zero_block(node->single_indirect);
            write_inode(inode_idx, node);
        }
        if (node->single_indirect == 0) return 0;
        
        block_read(node->single_indirect, (uint8_t*)ptrs);
        if (ptrs[logical_idx] == 0 && do_alloc) {
            ptrs[logical_idx] = alloc_block();
            block_write(node->single_indirect, (uint8_t*)ptrs);
        }
        return ptrs[logical_idx];
    }
    logical_idx -= PTRS_PER_BLOCK;

    uint32_t double_limit = PTRS_PER_BLOCK * PTRS_PER_BLOCK;
    if (logical_idx < double_limit) {
        if (node->double_indirect == 0 && do_alloc) {
            node->double_indirect = alloc_block();
            zero_block(node->double_indirect);
            write_inode(inode_idx, node);
        }
        if (node->double_indirect == 0) return 0;
        uint32_t l1_idx = logical_idx / PTRS_PER_BLOCK;
        uint32_t l2_idx = logical_idx % PTRS_PER_BLOCK;
        block_read(node->double_indirect, (uint8_t*)ptrs);
        if (ptrs[l1_idx] == 0 && do_alloc) {
            ptrs[l1_idx] = alloc_block();
            zero_block(ptrs[l1_idx]);
            block_write(node->double_indirect, (uint8_t*)ptrs);
        }
        if (ptrs[l1_idx] == 0) return 0;
        uint32_t data_ptrs[PTRS_PER_BLOCK];
        block_read(ptrs[l1_idx], (uint8_t*)data_ptrs);
        if (data_ptrs[l2_idx] == 0 && do_alloc) {
            data_ptrs[l2_idx] = alloc_block();
            block_write(ptrs[l1_idx], (uint8_t*)data_ptrs);
        }
        return data_ptrs[l2_idx];
    }
    logical_idx -= double_limit;
    uint32_t triple_limit = PTRS_PER_BLOCK * PTRS_PER_BLOCK * PTRS_PER_BLOCK;
    if (logical_idx < triple_limit) {
        if (node->triple_indirect == 0 && do_alloc) {
            node->triple_indirect = alloc_block();
            zero_block(node->triple_indirect);
            write_inode(inode_idx, node);
        }
        if (node->triple_indirect == 0) return 0;
        uint32_t l1_idx = logical_idx / (PTRS_PER_BLOCK * PTRS_PER_BLOCK);
        uint32_t rem    = logical_idx % (PTRS_PER_BLOCK * PTRS_PER_BLOCK);
        uint32_t l2_idx = rem / PTRS_PER_BLOCK;
        uint32_t l3_idx = rem % PTRS_PER_BLOCK;
        uint32_t l1_ptrs[PTRS_PER_BLOCK];
        block_read(node->triple_indirect, (uint8_t*)l1_ptrs);
        if (l1_ptrs[l1_idx] == 0 && do_alloc) {
            l1_ptrs[l1_idx] = alloc_block();
            zero_block(l1_ptrs[l1_idx]);
            block_write(node->triple_indirect, (uint8_t*)l1_ptrs);
        }
        if (l1_ptrs[l1_idx] == 0) return 0;

        uint32_t l2_ptrs[PTRS_PER_BLOCK];
        block_read(l1_ptrs[l1_idx], (uint8_t*)l2_ptrs);
        if (l2_ptrs[l2_idx] == 0 && do_alloc) {
            l2_ptrs[l2_idx] = alloc_block();
            zero_block(l2_ptrs[l2_idx]);
            block_write(l1_ptrs[l1_idx], (uint8_t*)l2_ptrs);
        }
        if (l2_ptrs[l2_idx] == 0) return 0;

        uint32_t l3_ptrs[PTRS_PER_BLOCK];
        block_read(l2_ptrs[l2_idx], (uint8_t*)l3_ptrs);
        if (l3_ptrs[l3_idx] == 0 && do_alloc) {
            l3_ptrs[l3_idx] = alloc_block();
            block_write(l2_ptrs[l2_idx], (uint8_t*)l3_ptrs);
        }
        
        return l3_ptrs[l3_idx];
    }
    return 0;
}

uint32_t fs_read(uint32_t inode_idx, inode_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    uint32_t bytes_read = 0;
    uint8_t sector_buf[512];
    if (offset + size > node->size) {
        size = node->size - offset;
    }
    while (bytes_read < size) {
        uint32_t current_global_offset = offset + bytes_read;
        uint32_t block_index = current_global_offset / 512;
        uint32_t offset_in_block = current_global_offset % 512;
        uint32_t phys_block = get_physical_block(inode_idx, node, block_index, 0);
        if (phys_block == 0) {
            memset(sector_buf, 0, 512);
        } else {
            block_read(phys_block, sector_buf);
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
    if (b == 0) {
        free_inode(idx);
        return (uint32_t)-1;
    }
    node.direct[0] = (uint32_t)b;
    write_inode(idx, &node);
    inode_t root;
    read_inode(0, &root);
    dir_add(0, &root, name, (uint32_t)idx);
    return (uint32_t)idx;
}
