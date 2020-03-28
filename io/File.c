#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<string.h>

#include "../disk/disk.h"
#include "ffs_helper.h"

static const int NUM_DIRECT_PTRS = 12;
int ROOT_INODE_INDEX = 2;
const int DIRECTORY_ENTRY_SIZE = 22;

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
    _2_byte_int inode_num; // 2
    char file_name[DIRECTORY_ENTRY_SIZE - 2]; // 20
    int filename_size;

    char entry[DIRECTORY_ENTRY_SIZE + 1]; // 20 + 2 + 1 (null)

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

void convert_inode_to_dblock(inode* i)
{
    int str_idx = 0;
    strcpy(i->inode_str, "");

    char a[5] = "";
    char b[5] = "";
    convert_int_to_4byte_char(i->size, a);
    convert_int_to_4byte_char(i->is_dir, b);

    strncat(i->inode_str, a, 4);
    strncat(i->inode_str, b, 4);

    str_idx = 8;
    for(int j = 0; j<NUM_DIRECT_PTRS; j++)
    {
        for(int k = 0; k<2; k++)
            i->inode_str[str_idx++] = i->dblocks_idx[j].b_num[k];
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

void convert_direntry_to_str(dir_entry* dir_arr, int dir_size)
{
    for(int i = 0; i<dir_size; i++)
    {
        convert_int_to_binary_char(dir_arr[i].inode_num.i_num, dir_arr[i].inode_num.b_num);
        for(int j = 0; j<DIRECTORY_ENTRY_SIZE; j++)
        {
            // inode_num
            if (j<2)
                dir_arr[i].entry[j] = dir_arr[i].inode_num.b_num[j];

            else if(j - 2 < dir_arr[i].filename_size)
                dir_arr[i].entry[j] = dir_arr[i].file_name[j - 2];
            else
                dir_arr[i].entry[j] = '\0';
        }

        dir_arr[i].entry[DIRECTORY_ENTRY_SIZE] = '\0';
    }
}

void parse_dir_from_db(dir_entry* new_dir, char* db, int dir_size)
{
    int entry_idx = 0;
    int char_idx = 0;

    for(int i = 0; i<dir_size*DIRECTORY_ENTRY_SIZE; i++)
    {
        // next dir_entry
        if(char_idx == DIRECTORY_ENTRY_SIZE)
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

/**
 * if str1 = /usr/local/bin
 * 
 * @return: str1 = usr, str2 = local/bin
 */
void strtok_dir_path_1level(char** str1, char** str2)
{
    if(**str1 == '/')
        *str1 = *str1 + 1;

    char* t = *str1;

    while(**str1 != '\0' && **str1 != '/')
        *str1 = *str1 + 1;
    
    if (**str1 == '\0')
    {
        *str1 = t;
        *str2 = NULL;
    }
    else
    {
        *str2 = *str1 + 1;
        **str1 = '\0';
        *str1 = t;
    }
}

inode walk_down_dir(int inode_idx, char* dir_path)
{

    inode new_inode;

    // load inode
    read_blocks_from_disk(1, &inode_idx, new_inode.inode_str);
    parse_inode(&new_inode, inode_idx); // dblock str to inode struct

    // go through all dblocks
    for(int i = 0; new_inode.dblocks_idx[i].i_num != 0; i++)
    {
        // load db
        char new_db[VDISK_BLOCK_SIZE_BYTES];
        read_blocks_from_disk(1, &new_inode.dblocks_idx[i].i_num, new_db);

        int num_dir_entries;
        if(new_inode.size < VDISK_BLOCK_SIZE_BYTES/DIRECTORY_ENTRY_SIZE)
            num_dir_entries = new_inode.size;
        else
            num_dir_entries = VDISK_BLOCK_SIZE_BYTES/DIRECTORY_ENTRY_SIZE; // max possible # dir par block

        // load the contents of this dir (per block) TODO maybe try all blocks at once
        dir_entry new_dir_arr[num_dir_entries];
        parse_dir_from_db(new_dir_arr, new_db, new_inode.size);

        char dir_cpy[25] = "";
        strcpy(dir_cpy, dir_path);

        char* str1 = dir_cpy;
        char* str2;

        strtok_dir_path_1level(&str1, &str2);

        // /usr/local/bin
        // usr, local/bin
        int j=0;
        while(j<num_dir_entries && strcmp(new_dir_arr[j].file_name, str1) != 0)
            j++;

        if (j >= num_dir_entries && (i+1<12 && new_inode.dblocks_idx[i+1].i_num != 0)) // try next db if there is one
            continue;
        else if(str2 != NULL) // if haven't run out of entries
            return walk_down_dir(new_dir_arr[j].inode_num.i_num, str1);
        else
            return new_inode;

    }
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

    convert_inode_to_dblock(&new_inode);
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
    // get inode for new dir
    inode new_dir_inode = create_empty_inode(1);
    new_dir_inode.inode_db_idx = get_next_free_block();
    unset_nth_bit(new_dir_inode.inode_db_idx);
    new_dir_inode.size = 0;

    /* initialize dir stuff */
    dir_entry new_dir;

    char path_str_cpy[100];

    int last_slash = 0;
    for(new_dir.filename_size = 0; path_str[new_dir.filename_size] != '\0'; new_dir.filename_size++)
        if(path_str[new_dir.filename_size] == '/')
            last_slash = new_dir.filename_size;

    int idx = 0;
    for(int i = last_slash + 1; i<new_dir.filename_size; i++)
        new_dir.file_name[idx++] = path_str[i];
    
    new_dir.file_name[idx] = '\0';
    new_dir.filename_size = idx;

    new_dir.inode_num.i_num = new_dir_inode.inode_db_idx;
    convert_direntry_to_str(&new_dir, 1);
    /* end dir stuff */

    /* append to parent's dblock */
    // if (strcmp(path_str, "/usr/local") == 0)
    int parent_inode_idx = ROOT_INODE_INDEX; // always start at root
    inode parent_inode = walk_down_dir(parent_inode_idx, path_str); // TODO: instead of root, use walk_dir

    char parent_dir_block[512];
    read_blocks_from_disk(1, &parent_inode.dblocks_idx[(parent_inode.size * DIRECTORY_ENTRY_SIZE)/VDISK_BLOCK_SIZE_BYTES].i_num, parent_dir_block);

    int parent_append_idx = parent_inode.size * DIRECTORY_ENTRY_SIZE;
    for(int i = 0; i<DIRECTORY_ENTRY_SIZE; i++)
        parent_dir_block[parent_append_idx++] = new_dir.entry[i];

    /* end append*/

    parent_inode.size++;

    /* write back parent */
    convert_inode_to_dblock(&parent_inode);
    write_block_to_disk(parent_inode.inode_db_idx, parent_inode.inode_str, 32);
    write_block_to_disk(parent_inode.dblocks_idx[0].i_num, parent_dir_block, DIRECTORY_ENTRY_SIZE * parent_inode.size);

    /* write dir dblock */
    new_dir_inode.dblocks_idx[0].i_num = get_next_free_block(); // tell dir_inode where the block is at
    unset_nth_bit(new_dir_inode.dblocks_idx[0].i_num);
    convert_int_to_binary_char(new_dir_inode.dblocks_idx[0].i_num, new_dir_inode.dblocks_idx[0].b_num);

    new_dir_inode.size = 0;

    char db[VDISK_BLOCK_SIZE_BYTES] = "";
    write_block_to_disk(new_dir_inode.dblocks_idx[0].i_num, db, 0);

    // write directory's inode
    convert_inode_to_dblock(&new_dir_inode);
    write_block_to_disk(new_dir_inode.inode_db_idx, new_dir_inode.inode_str, 32);

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


    // root db
    char root_db[512] = "";
    root_inode.dblocks_idx[0].i_num = get_next_free_block();
    unset_nth_bit(root_inode.dblocks_idx[0].i_num);
    convert_int_to_binary_char(root_inode.dblocks_idx[0].i_num, root_inode.dblocks_idx[0].b_num);

    // 0 files in root rn
    root_inode.size = 0;

    // write root inode to disk
    convert_inode_to_dblock(&root_inode);
    write_block_to_disk(ROOT_INODE_INDEX, root_inode.inode_str, 32);

    // root db to disk
    write_block_to_disk(root_inode.dblocks_idx[0].i_num, root_db, 0);

}

int main()
{
    initLLFS();
    create_root_dir_on_disk();

    mk_dir("/usr");
    mk_dir("/whatev");
    mk_dir("/usr/hello");
    mk_dir("/usr/sup");
    mk_dir("/usr/lol");

    return 0;
}