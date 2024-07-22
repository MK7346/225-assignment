#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

#define BLOCK_SIZE 4096
#define INODE_TABLE_SIZE 128
#define TOTAL_BLOCKS 1024
#define TOTAL_INODES 128
#define MAX_JOURNAL_ENTRIES (BLOCK_SIZE /sizeof(struct journal_entry))
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
//journal entry structure
struct journal_entry{
    unsigned int op; //for operation bing done
    unsigned int inode_index;
    unsigned int block_index;
    char data[BLOCK_SIZE];

};


//we declare 2 Global Variables for Superblock and Inode Table
struct superblock sb;
struct inode *inode_table;
//the following is for the read adn write operations
unsigned char disk[TOTAL_BLOCKS][BLOCK_SIZE];
struct journal_entry journal[MAX_JOURNAL_ENTRIES];
unsigned int journal_entry_count =0;

//prototype of functions
void write_block(int block_index, void *data);
void read_block(int block_index, void *data);
void initialize_filesystem();
int allocate_inode();
void deallocate_inode(int inode_index);
int allocate_block();
void deallocate_block(int block_index);
void create_root_directory();
int create_file(const char *filename, int parent_inode_index);
int write_file(int inode_index, const char *data, int size);
int read_file(int inode_index, char *buffer, int size);
void list_directory(int inode_index);
void log_journal_entry(unsigned int op, unsigned int inode_index, unsigned int block_index, const char *data);
void recover_filesystem();
void apply_journal_entry(struct journal_entry *entry);

//for write data to a block
void write_block(int block_index, void *data){
    if(block_index<0 || block_index >= TOTAL_BLOCKS){
        printf("block index is invalid \n");
        return;
    }
    memcpy(disk[block_index],data, BLOCK_SIZE);
}

//reading data from a block
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

            //log journal the emtry before writing to the disk
            log_journal_entry(1, inode_index, parent_inode->i_block[0], (const char *)parent_dir);

            break;
        }
        entry_offset += entry->rec_len;
    }

    // Write parent directory block back
    write_block(parent_inode->i_block[0], parent_dir);

    free(parent_dir);
    return inode_index;
}

// for writting to a file
int write_file(int inode_index, const char *data, int size){
    struct inode *file_inode= &inode_table[inode_index];
    int block_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (block_needed > 15){
        printf("the file size exceeds maximun limit. \n");
        return -1;
    }

    for (int i=0; i <block_needed; i++){
        int block_index = allocate_block();
        if (block_index == -1){
            printf("allocation of file failed. \n");
            return -1;
        }
        file_inode->i_block[i] = block_index;
        
        //log journal entry before writing it to disk
        log_journal_entry(2, inode_index, block_index, data + i *BLOCK_SIZE);

        write_block(block_index, (void *)(data + i *BLOCK_SIZE));

    }
    file_inode->i_size = size;
    file_inode->i_mtime = time(NULL);
    file_inode->i_blocks =block_needed;
    return 0;
}

// for reading from a file
int read_file(int inode_index, char *buffer, int size){
    struct inode *file_inode = &inode_table[inode_index];
    int block_reading = (size + BLOCK_SIZE -1 ) /BLOCK_SIZE;
    if (block_reading>file_inode->i_blocks){
        printf("the size requested exceeds file size. \n");
        return -1;
    }
    for (int i=0; i<block_reading; i++){
        read_block(file_inode->i_block[i], (void *)(buffer + i *BLOCK_SIZE));
    }
    return 0;
}

// this one is for directory listing 
void list_directory(int inode_index){
    struct inode *dir_inode = &inode_table[inode_index];
    
    if ((dir_inode->i_mode & 0x4000) == 0){
        printf("inode %d is not a directory \n",inode_index);
        return;
    }
    struct directory_entry *dir_entries = (struct direrctory_entry *)malloc(BLOCK_SIZE);
    read_block(dir_inode->i_block[0], dir_entries);

    printf("listing directory inode %d: \n", inode_index);
    int entry_offset =0;
    while (entry_offset < BLOCK_SIZE){
        struct directory_entry *entry = (struct directory_entry *)((char *)dir_entries + entry_offset);
        if(entry->inode !=0){
            printf("inode: %d, name: %.*s\n", entry->inode, entry->name_len,entry->name);
        }
        entry_offset += entry->rec_len;
    }
    free(dir_entries);
}
// logging journal entries 
void log_journal_entry(unsigned int op, unsigned int inode_index, unsigned int block_inex, const char *data){
    if (journal_entry_count >= MAX_JOURNAL_ENTRIES){
        printf("journal is full thus it can't log the new entry. \n");
        return;
    }
    struct journal_entry *entry = &journal[journal_entry_count++];
    entry->op =op;
    entry->inode_index = inode_index;
    entry->block_index = block_index;
    if (data){
        memcpy(entry->data, data, BLOCK_SIZE);
    }
    else{
        memset(entry->data, 0, BLOCK_SIZE);
    }
}

void apply_journal_entry(struct journal_entry *entry){
    switch (entry->op)
    {
    case 1: //create of file
        // will be added later
        break;
    case 2: //for writing 
        write_block(entry->block_index, entry->data);
        break;
    case 3: //delete
        //latttormregp
        break;
    default:
        printf("unknown journal operation: %u \n", entry->op);
    }
}
// recovery
void recover_filesystem(){
    for(unsigned int i =0; i<journal_entry_count; i++){
        apply_journal_entry(&journal[i]);
    }
    journal_entry_count = 0; // for emptying the journal after recover
}

int main()
{
    initialize_filesystem();
    printf("File system initialized successfully.\n");

    // Create a new file in the root directory
    int file_inode_index = create_file("example.txt", 0);
    if (file_inode_index >= 0) {
        printf("File 'example.txt' created successfully with inode index %d.\n", file_inode_index);
        const char *data = "Hello, this is a test file.";
        int write_status = write_file(file_inode_index, data, strlen(data));
        if (write_status == 0) {
            printf("Data written to 'example.txt' successfully.\n");
        } else if (write_status == 1) {
            printf("Failed to write data to 'example.txt': Allocation error.\n");
        } else if (write_status == 2) {
            printf("Failed to write data to 'example.txt': Size limit error.\n");
        }

        char buffer[BLOCK_SIZE];
        int read_status = read_file(file_inode_index, buffer, strlen(data));
        if (read_status == 0) {
            printf("Data read from 'example.txt': %s\n", buffer);
        } else {
            printf("Failed to read data from 'example.txt'.\n");
        }

        // List the root directory contents
        list_directory(0);
    } else {
        printf("Failed to create file 'example.txt'. Error code: %d\n", file_inode_index);
    }
    return 0;
}

