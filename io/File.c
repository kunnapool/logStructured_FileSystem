#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<string.h>

#include "../disk/disk.h"
#include "ffs_helper.h"

static const int NUM_DIRECT_PTRS = 12;
int ROOT_INODE_INDEX = 2;
int DIRECTORY_ENTRY_SIZE = 22;

typedef struct two_byte_ints
{
    char b_num[2];
    int i_num;

}_2_byte_int;

typedef struct inodes
{
    int size; // TODO maybe change to _2_byte_int
    int is_dir; // ^same

    int inode_db_idx;

    _2_byte_int dblocks_idx[12]; // direct pointers, 2 bytes each

    char inode_str[VDISK_BLOCK_SIZE_BYTES];

}inode;

typedef struct directory_entry
{
    _2_byte_int inode_num;
    char file_name[20];

    char entry[22];

} dir_entry;

inode ROOT_INODE;

void convert_int_to_4byte_char(int i, char* c)
{
    int t = i;
    int num_d = 0;

    // num digits in int
    while(t != 0)
    {
        num_d++;
        t /= 10;
    }

    char o[5] = "";
    for(int k = 0; k< 4 - num_d; k++)
        strcat(o, "0");


    strcpy(c, "");
    strcat(c, o);

    if (num_d != 0)
    {
        char n[5] = "";
        sprintf(n, "%d", i);
        strcat(c, n);
    }

}

void convert_inode_to_dblock(inode i, char* inode_str)
{   

    char* str = inode_str;
    int str_idx = 0;
    strcpy(str, "");

    char a[5] = "";
    char b[5] = "";
    convert_int_to_4byte_char(i.size, a);
    convert_int_to_4byte_char(i.is_dir, b);

    strncat(str, a, 4);
    strncat(str, b, 4);

    str_idx = 8;
    for(int j = 0; j<NUM_DIRECT_PTRS; j++)
    {
        for(int k = 0; k<2; k++)
            str[str_idx++] = i.dblocks_idx[j].b_num[k];
    }

}

inode create_empty_inode(int is_dir)
{
    inode new_inode;

    new_inode.is_dir = is_dir;
    new_inode.size = 0;

    // set all dblock pointers to null
    for(int i = 0; i<NUM_DIRECT_PTRS; i++)
    {
        new_inode.dblocks_idx[i].i_num = 0; // consider 0 as invalid/free
        convert_int_to_binary_char(new_inode.dblocks_idx[i].i_num, new_inode.dblocks_idx[i].b_num);
    }

    return new_inode;
}

void parse_inode(inode* ind, int inode_db_idx)
{
    char c_num[5] = "";

    for(int i = 0; i<4; i++)
        c_num[i] = ind->inode_str[i];
    c_num[4] = '\0';

    int i_size = atoi(c_num);

    strcpy(c_num, "");

    for(int i = 4; i<8; i++)
        c_num[i-4] = ind->inode_str[i];
    c_num[4] = '\0';

    int i_isdir = atoi(c_num);

    ind->size = i_size;
    ind->is_dir = i_isdir;

    // dblock pointers
    for(int i = 0; i<NUM_DIRECT_PTRS*2; i++)
        ind->dblocks_idx[i/2].b_num[i%2] =  ind->inode_str[i + 8];
    
    for(int i = 0; i<NUM_DIRECT_PTRS; i++)
        ind->dblocks_idx[i].i_num =  convert_binary_char_to_int(ind->dblocks_idx[i].b_num);

    ind->inode_db_idx = inode_db_idx;
}

void convert_direntry_to_db(dir_entry* dir_arr, int dir_size)
{
    for(int i = 0; i<dir_size; i++)
    {
        for(int j = 0; j<DIRECTORY_ENTRY_SIZE; j++)
        {
            // inode_num
            if (j<2)
                dir_arr[i].entry[j] = dir_arr[i].inode_num.b_num[j];

            dir_arr[i].entry[j] = dir_arr[i].file_name[j-2];
        }
    }
}

void parse_dir(dir_entry* new_dir, char* db, int dir_size)
{
    int entry_idx = 0;
    int char_idx = 0;

    for(int i = 0; i<dir_size*DIRECTORY_ENTRY_SIZE; i++)
    {
        // next dir_entry
        if(char_idx == DIRECTORY_ENTRY_SIZE - 1)
        {
            char_idx = 0;
            entry_idx++;
        }

        // first two bytes are inode num
        if(char_idx < 2)
            new_dir[entry_idx].inode_num.b_num[char_idx] = db[i];
        else
            new_dir[entry_idx].file_name[char_idx - 2] = db[i];
        
        char_idx++;
    }

    for(int i = 0; i<dir_size; i++)
        new_dir[i].inode_num.i_num = convert_binary_char_to_int(new_dir[i].inode_num.b_num);

}

inode walk_down_dir(int inode_idx, char* dir_path)
{
    printf("hey\n");
    inode new_inode;

    // load inode
    read_blocks_from_disk(1, &inode_idx, new_inode.inode_str);
    parse_inode(&new_inode, inode_idx); // dblock str to inode struct

    // load db
    char new_db[VDISK_BLOCK_SIZE_BYTES];
    read_blocks_from_disk(1, &new_inode.dblocks_idx[0].i_num, new_db);
    int num_nonzero_blocks = (new_inode.size / 512) + 1;

    dir_entry new_dir_arr[new_inode.size];
    parse_dir(new_dir_arr, new_db, new_inode.size);

    return new_inode;
}

void create_file(char* file_str, int file_str_size) //TODO parent_what???
{
    int next_free_db = get_next_free_block();
    unset_nth_bit(next_free_db);
    inode new_inode = create_empty_inode(0);

    new_inode.inode_db_idx = next_free_db;

    next_free_db = get_next_free_block(); // for file db

    char new_db[512];

    for(int i = 0; i<file_str_size; i++)
        new_db[i] = file_str[i];
    
    new_inode.size = file_str_size;

    // TODO dblocks_idx[ new_node.size % 512 ]
    new_inode.dblocks_idx[0].i_num = next_free_db;
    convert_int_to_binary_char(new_inode.dblocks_idx[0].i_num, new_inode.dblocks_idx[0].b_num);
    unset_nth_bit(next_free_db);

    convert_inode_to_dblock(new_inode, new_inode.inode_str);
    write_block_to_disk(new_inode.inode_db_idx, new_inode.inode_str, 32);

    write_block_to_disk(next_free_db, new_db, new_inode.size);

}

void concat_binarychar_str(char c, char* str, int str_size)
{
    // shift things one place
    for(int i = str_size; i>0; i--)
        str[i] = str[i-1];
    
    str[0] = c;
}

void mk_dir(char* path_str)
{
    inode new_inode = create_empty_inode(1);
    new_inode.inode_db_idx = get_next_free_block();
    unset_nth_bit(new_inode.inode_db_idx);

    inode root_inode = walk_down_dir(ROOT_INODE_INDEX, ""); // TODO instead of root, use walk_dir

    dir_entry new_dir;

    strcpy(new_dir.file_name, "usr");
    new_dir.inode_num.i_num = 69;

    // read and append into parent db
    char parent_dir_block[512];
    read_blocks_from_disk(1, &root_inode.dblocks_idx[0].i_num, parent_dir_block);

    /* append to parent's dblock */
    convert_int_to_binary_char(new_dir.inode_num.i_num, new_dir.inode_num.b_num);
    parent_dir_block[root_inode.size] = new_dir.inode_num.b_num[0];
    parent_dir_block[root_inode.size + 1] = new_dir.inode_num.b_num[1];

    for(int i = root_inode.size + 2; i<root_inode.size + 2 + 3; i++)
        parent_dir_block[i] = new_dir.file_name[i - root_inode.size - 2];
    /* end append*/

    root_inode.size += 2 + 3;

    // write back parent's inode
    convert_inode_to_dblock(root_inode, root_inode.inode_str);
    write_block_to_disk(ROOT_INODE_INDEX, root_inode.inode_str, 32);
    
    // write back parent dblock
    write_block_to_disk(root_inode.dblocks_idx[0].i_num, parent_dir_block, root_inode.size);

    // write directory's inode
    convert_inode_to_dblock(new_inode, new_inode.inode_str);
    write_block_to_disk(new_inode.inode_db_idx, new_inode.inode_str, 32);

    // write directory's dblock

}

/**
 * Create empty root directory with an inode and an
 * empty dblock on disk
 */
void create_root_dir_on_disk()
{
    inode root_inode;

    root_inode = create_empty_inode(1);
    root_inode.inode_db_idx = ROOT_INODE_INDEX;

    dir_entry root_dir;

    strcpy(root_dir.file_name, "");
    root_inode.size = 0;

    // write root inode to disk
    convert_inode_to_dblock(root_inode, root_inode.inode_str);
    write_block_to_disk(ROOT_INODE_INDEX, root_inode.inode_str, 32);

    // root db
    char root_db[512] = "";
    root_inode.dblocks_idx[0].i_num = get_next_free_block();
    unset_nth_bit(root_inode.dblocks_idx[0].i_num);
    convert_int_to_binary_char(root_inode.dblocks_idx[0].i_num, root_inode.dblocks_idx[0].b_num);

    // 0 files in root rn
    root_inode.size = 0;

    // root db to disk
    write_block_to_disk(root_inode.dblocks_idx[0].i_num, root_db, 0);

}

int main()
{
    initLLFS();
    create_root_dir_on_disk();

    mk_dir("");

    // walk_down_dir(ROOT_INODE_INDEX, "");

    return 0;
}