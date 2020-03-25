#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<string.h>

#include "../disk/disk.h"
#include "ffs_helper.h"

static const int NUM_DIRECT_PTRS = 12;
static const int ROOT_INODE_INDEX = 2;

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

inode create_inode(int is_dir)
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

inode parse_inode(inode* ind)
{
    char size[5] = "";
    ind->inode_str[]
}

void walk_down_dir(char* dir_path)
{
    inode root_inode;
    read_blocks_from_disk(1, &ROOT_INODE_INDEX, root_inode.inode_str);


}

void create_file(char* file_str, int file_str_size) //TODO parent_what???
{
    int next_free_db = get_next_free_block();
    inode new_inode = create_inode(0);

    new_inode.inode_db_idx = next_free_db;

    // update free-block vector - inode shall go here
    unset_nth_bit(next_free_db);

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

void mk_dir()
{
    char dir_format[21];

    char inode_num[2];
    convert_int_to_binary_char(212, inode_num);

    dir_format[0] = inode_num[0];
    dir_format[1] = inode_num[1];
    int dir_idx = 2;

    char dir_name[] = "My new directory";
    for(int i = 0; i<21 - dir_idx || dir_name[i] != '\0'; i++)
        dir_format[dir_idx++] = dir_name[i];
    
    create_file(dir_format, 21);
}

void create_root_dir_on_disk()
{
    inode root_inode;

    root_inode = create_inode(1);
    root_inode.inode_db_idx = ROOT_INODE_INDEX;

    convert_inode_to_dblock(root_inode, root_inode.inode_str);
    write_block_to_disk(ROOT_INODE_INDEX, root_inode.inode_str, 32);

}


int main()
{
    initLLFS();
    create_root_dir_on_disk();

    // create_file("This is a newly created file y'all");
    mk_dir();

    return 0;
}