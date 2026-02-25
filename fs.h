#ifndef FS_H
#define FS_H
#define INODE_DIRECT 6
#define MAX_TAGS 3
#define TAG_LEN 12
#include <stdint.h>
#include <stddef.h>

#define BLOCK_BITMAP_MAX_SIZE 8192
extern uint8_t *block_bitmap;
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

//Structure of a file/(folder)
typedef struct {
    uint32_t size;    
    uint32_t direct[INODE_DIRECT]; 
    uint8_t  type;         
    uint8_t  reserved[11];  
    char main_tag[TAG_LEN];
    char tags[MAX_TAGS][TAG_LEN];
    uint8_t unused[3];
} inode_t;

int fs_find_by_tag(const char* tag, uint32_t* results, int max_results);

// helper declarations that need `inode_t`
void free_block(uint32_t idx);
void free_inode(int idx);
int dir_remove(inode_t* dir, const char* name);
void block_read(uint32_t lba, uint8_t* buf);
void block_write(uint32_t lba, const uint8_t* buf);
void read_inode(int idx, inode_t* inode);
void write_inode(int idx, inode_t* inode);

/* file API (declared in fs.c) */
uint32_t fs_create_file(const char* name, const char* main_tag);
int fs_write(uint32_t inode, const uint8_t* data, size_t len);
int fs_read(uint32_t inode_idx, inode_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
int fs_delete_file(const char* path);
int dir_lookup(inode_t* dir, const char* name);
//int dir_add(inode_t* dir, const char* name, uint32_t inode);
int dir_add(uint32_t dir_inode_id, inode_t* dir, const char* name, uint32_t inode_idx);

#endif