#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

#define BLOCK_SIZE 4096
#define INODE_TABLE_SIZE 128
#define TOTAL_BLOCKS 1024
#define TOTAL_INODES 128

//what is below is the superblock
struct superblock{
    unsigned int s_inodes_count;
    unsigned int s_blocks_count;
    unsigned int s_free_blocks_count;
    unsigned int s_free_inodes_count;
    unsigned int s_block_size;
    unsigned int s_blocks_per_group;
    unsigned int s_inodes_per_group;
    unsigned int s_journal_block;
    unsigned int s_feature_compat;
    unsigned int s_feature_incompat;
    unsigned int s_feature_ro_compact;

};
// we now make the structure of the I-nodes
struct inode {
    unsigned short i_mode;
    unsigned short i_uid;
    unsigned int i_size;
    unsigned int i_atime;
    unsigned int i_ctime;
    unsigned int i_mtime;
    unsigned int i_dtime;
    unsigned short i_gid;
    unsigned short i_links_count;
    unsigned int i_blocks;
    unsigned int i_block[15];
    unsigned int i_flags;
    unsigned int i_osd1;
    unsigned int i_generation;
    unsigned int i_file_acl;
    unsigned int i_dir_acl;
    unsigned int i_faddr;
    unsigned int i_osd2[3];

};
//we now define the directory entry structure

struct directory_entry {
    unsigned int inode;
    unsigned short rec_len;
    unsigned char name_len;
    unsigned char file_type;
    char name[255];

};

//we declare 2 Global Variables for Superblock and Inode Table
struct superblock sb;
struct inode *inode_table;

void filesystem_initialization(){
    //the following initializes the superblock
    sb.s_inodes_count = TOTAL_INODES;
    sb.s_blocks_count = TOTAL_BLOCKS;
    sb.s_free_blocks_count = TOTAL_BLOCKS;
    sb.s_free_inodes_count = TOTAL_INODES;
    sb.s_block_size = BLOCK_SIZE;
    sb.s_blocks_per_group = TOTAL_BLOCKS / 8; // Example value
    sb.s_inodes_per_group = TOTAL_INODES / 8; // ""
    sb.s_journal_block = TOTAL_BLOCKS - 1; // Example journal block position
    sb.s_feature_compat = 0;
    sb.s_feature_incompat = 0;
    sb.s_feature_ro_compact = 0;

    // Allocate and initialize the inode table
    inode_table = (struct inode *)malloc(INODE_TABLE_SIZE * sizeof(struct inode));
    memset(inode_table, 0, INODE_TABLE_SIZE * sizeof(struct inode));
}

int main()
{
    filesystem_initialization();
    printf("file stuff successful. \n");
    return 0;
}

