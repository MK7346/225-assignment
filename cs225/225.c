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
//the following is for the read adn write operations
unsigned char disk[TOTAL_BLOCKS][BLOCK_SIZE];

void write_block(int block_index, void *data){
    if(block_index<0 || block_index >= TOTAL_BLOCKS){
        printf("block index is invalid \n");
        return;
    }
    memcpy(disk[block_index],data, BLOCK_SIZE);
}
void read_block(int block_index, void *data){
    if(block_index<0 || block_index>= TOTAL_BLOCKS){
        printf("block index is invalid \n");
        return;
    }
    memcpy(data, disk[block_index], BLOCK_SIZE);
}
//initialize the file system
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
//allocating an inode and returning its index
int allocate_inode(){
    for(int i=0; i< INODE_TABLE_SIZE;i++){
        if (inode_table[i].i_mode==0){
            inode_table[i].i_mode =1;
            sb.s_free_inodes_count--;
            return i;
        }
    }
    return -1;
}
//deallocating an inode
void deallocate_inode(int inode_index){
    inode_table[inode_index].i_mode =0;
    sb.s_free_inodes_count++;
}
//allpocating a block and returning its index
int allocate_block(){
    static int next_free_block=0;
    if (next_free_block<sb.s_blocks_count){
        sb.s_free_blocks_count--;
        return next_free_block++;
    }
    return -1;
}
//deallocate a block
void deallocate_block(int block_index){
    sb.s_free_blocks_count++;
}
//CREATING A ROOT DIRECTORY
void create_root_directory(){
    int root_inode_index = allocate_inode();
    if (root_inode_index == -1){
        printf("allocation of inode for rrot directory failed.\n");
        exit(1);
    }
    struct inode *root_inode = &inode_table[root_inode_index];
    root_inode->i_mode = 0x4000; // this sets the mode bits of an inode to mark it as a directory
    root_inode->i_uid =0;
    root_inode->i_gid =0;
    root_inode->i_size = BLOCK_SIZE;
    root_inode->i_atime = root_inode->i_ctime = root_inode->i_mtime = time(NULL);
    root_inode->i_links_count =2;
    root_inode->i_blocks =1;
    
    int block_index = allocate_block();
    if (block_index == -1){
        printf("allocation of block for root directory failed.\n");
        exit(1);
    }
    root_inode->i_block[0]=block_index;

    struct directory_entry *root_dir =(struct directory_entry *)malloc(BLOCK_SIZE);
    memset(root_dir, 0, BLOCK_SIZE);
    root_dir[0].inode = root_inode_index;
    root_dir[0].rec_len = sizeof(struct directory_entry);
    root_dir[0].name_len =1;
    root_dir[0].file_type =2;
    strcpy(root_dir[0].name, ".");
    root_dir[1].inode = root_inode_index;
    root_dir[1].rec_len = sizeof(struct directory_entry);
    root_dir[1].name_len = 2;
    root_dir[1].file_type =2;
    strcpy(root_dir[1].name, "..");

    //write the root directry to block
    write_block(block_index, root_dir);
    free(root_dir);

}
//file creation
int create_file(const char *filename, int parent_inode_index) {
    int inode_index = allocate_inode();
    if (inode_index == -1) {
        printf("allocation of inode for file failed.\n");
        return -1;
    }

    struct inode *file_inode = &inode_table[inode_index];
    file_inode->i_mode = 0x8000; // Regular file
    file_inode->i_uid = 0;
    file_inode->i_gid = 0;
    file_inode->i_size = 0;
    file_inode->i_atime = file_inode->i_ctime = file_inode->i_mtime = time(NULL);
    file_inode->i_links_count = 1;
    file_inode->i_blocks = 0;

    struct inode *parent_inode = &inode_table[parent_inode_index];
    struct directory_entry *parent_dir = (struct directory_entry *)malloc(BLOCK_SIZE);
    memset(parent_dir, 0, BLOCK_SIZE);

    // Read parent directory block
    read_block(parent_inode->i_block[0], parent_dir);

    int entry_offset = 0;
    while (entry_offset < BLOCK_SIZE) {
        struct directory_entry *entry = (struct directory_entry *)((char *)parent_dir + entry_offset);
        if (entry->inode == 0) { // Found a free entry
            entry->inode = inode_index;
            entry->rec_len = sizeof(struct directory_entry);
            entry->name_len = strlen(filename);
            entry->file_type = 1; // Regular file
            strcpy(entry->name, filename);
            break;
        }
        entry_offset += entry->rec_len;
    }

    // Write parent directory block back
    write_block(parent_inode->i_block[0], parent_dir);

    free(parent_dir);
    return inode_index;
}


int main()
{
    filesystem_initialization();
    printf("file stuff successful. \n");
    // Create a new file in the root directory
    int file_inode_index = create_file("example.txt", 0);
    if (file_inode_index != -1) {
        printf("File 'example.txt' created successfully with inode index %d.\n", file_inode_index);
    } else {
        printf("Failed to create file 'example.txt'.\n");
    }
    return 0;
}

