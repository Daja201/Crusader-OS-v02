#ifndef FS_H
#define FS_H
#include <stdint.h>
#include <stddef.h>

#define BLOCK_BITMAP_MAX_SIZE 8192
extern uint8_t *block_bitmap;

#define INODE_DIRECT 8
//
//Header for filesystem same as when you were making FAT12

void init_fs(void);

#define INODE_BITMAP_SIZE 512
extern uint8_t inode_bitmap[INODE_BITMAP_SIZE];



void load_block_bitmap(void);
void save_block_bitmap(void);
void load_inode_bitmap(void);
void save_inode_bitmap(void);
int alloc_block(void);
void create_root(void);
// helper declarations requiring `inode_t` are below

typedef struct {
    uint32_t magic;
    uint32_t block_size;
    uint64_t total_blocks;
    uint32_t inode_count;
    uint32_t bitmap_start;
    uint32_t inode_start;
    uint32_t data_start;
} superblock_t;

// Exposed global superblock (defined in fs.c)
extern superblock_t g_superblock;

typedef struct {
    uint32_t size;    
    uint32_t direct[INODE_DIRECT]; 
    uint8_t  type;         
    uint8_t  reserved[11];  
} inode_t;
// helper declarations that need `inode_t`
void free_block(uint32_t idx);
void free_inode(int idx);
int dir_remove(inode_t* dir, const char* name);
void block_read(uint32_t lba, uint8_t* buf);
void block_write(uint32_t lba, const uint8_t* buf);
void read_inode(int idx, inode_t* inode);
void write_inode(int idx, inode_t* inode);

/* file API (declared in fs.c) */
uint32_t fs_create_file(const char* path);
int fs_write(uint32_t inode, const uint8_t* data, size_t len);
int fs_read(uint32_t inode, uint8_t* buf, size_t len);
int fs_delete_file(const char* path);
int dir_lookup(inode_t* dir, const char* name);
//int dir_add(inode_t* dir, const char* name, uint32_t inode);
int dir_add(uint32_t dir_inode_id, inode_t* dir, const char* name, uint32_t inode_idx);

#endif