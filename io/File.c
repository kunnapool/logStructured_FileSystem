#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<string.h>

#include "../disk/disk.h"
#include "ffs_helper.h"

static const int NUM_DIRECT_PTRS = 12;
int ROOT_INODE_INDEX = 2;
static const int DIRECTORY_ENTRY_SIZE = 22;
static const int VDISK_BLOCK_SIZE_BYTES = 512;

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

typedef struct LogEntry
{
    int vdisk_index;
    int data_size;
    char db[VDISK_BLOCK_SIZE_BYTES];
} LogEntry;

int LOG_SEG_IDX = 0;
const static int MAX_LOG_SEGMENT_SIZE = 10;
LogEntry LOG_SEGMENT_ARR[MAX_LOG_SEGMENT_SIZE];

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

void printLogSeg()
{
    for(int i = 0; i<LOG_SEG_IDX; i++)
        printf("%d: %s\n", LOG_SEGMENT_ARR[i].vdisk_index, LOG_SEGMENT_ARR[i].db);
}

void read_from_segment_or_disk(int read_idx, char* read_db)
{
    for(int i = 0; i<LOG_SEG_IDX; i++)
        if(LOG_SEGMENT_ARR[i].vdisk_index == read_idx)
        {
            for(int j = 0; j<LOG_SEGMENT_ARR[i].data_size; j++)
                read_db[j] = LOG_SEGMENT_ARR[i].db[j];

            return;
        }

    read_blocks_from_disk(1, read_idx, read_db);
}

inode walk_down_dir(int inode_idx, char* dir_path)
{

    inode new_inode;

    // load inode
    read_from_segment_or_disk(inode_idx, new_inode.inode_str);
    parse_inode(&new_inode, inode_idx); // dblock str to inode struct

    // go through all dblocks
    for(int i = 0; new_inode.dblocks_idx[i].i_num != 0; i++)
    {
        // load db
        char new_db[VDISK_BLOCK_SIZE_BYTES];
        read_from_segment_or_disk(new_inode.dblocks_idx[i].i_num, new_db);

        int num_dir_entries;
        if(new_inode.size < VDISK_BLOCK_SIZE_BYTES/DIRECTORY_ENTRY_SIZE)
            num_dir_entries = new_inode.size;
        else
            num_dir_entries = VDISK_BLOCK_SIZE_BYTES/DIRECTORY_ENTRY_SIZE; // max possible # dir par block

        // load the contents of this dir (per block) TODO maybe try all blocks at once
        dir_entry new_dir_arr[num_dir_entries];
        parse_dir_from_db(new_dir_arr, new_db, num_dir_entries);

        char dir_cpy[25] = "";
        strcpy(dir_cpy, dir_path);

        char* str1 = dir_cpy;
        char* str2;

        strtok_dir_path_1level(&str1, &str2);
        // printf("dir_path: %s, str1: %s, str2: %s\n", dir_path, str1, str2);

        // /usr/local/bin
        // usr, local/bin
        int j=0;
        while(j<num_dir_entries && strcmp(new_dir_arr[j].file_name, str1) != 0)
            j++;

        if (j >= num_dir_entries && (i+1<12 && new_inode.dblocks_idx[i+1].i_num != 0)) // try next db if there is one
            continue;
        else if(str2 != NULL) // if haven't run out of entries
            return walk_down_dir(new_dir_arr[j].inode_num.i_num, str2);
        else
            return new_inode;

    }
    return new_inode;
}

void concat_binarychar_str(char c, char* str, int str_size)
{
    // shift things one place
    for(int i = str_size; i>0; i--)
        str[i] = str[i-1];
    
    str[0] = c;
}

void flush_log_segment()
{
    for(int i = 0; i<LOG_SEG_IDX; i++)
            write_block_to_disk(LOG_SEGMENT_ARR[i].vdisk_index, LOG_SEGMENT_ARR[i].db, LOG_SEGMENT_ARR[i].data_size);
        LOG_SEG_IDX = 0;
}

void append_log_segment(int db_idx, char* db, int data_size)
{
    // update existing block
    for(int i = 0; i<LOG_SEG_IDX; i++)
        if (LOG_SEGMENT_ARR[i].vdisk_index == db_idx)
        {
            // append to current block
            for(int j = 0; j<data_size; j++)
                LOG_SEGMENT_ARR[i].db[j] = db[j];

            // update size
            LOG_SEGMENT_ARR[i].data_size = data_size;

            // complete block
            for(int i = data_size; i<VDISK_BLOCK_SIZE_BYTES; i++)
                LOG_SEGMENT_ARR[LOG_SEG_IDX].db[i] = '\0';

            break;
        }

    // flush log
    if (LOG_SEG_IDX == MAX_LOG_SEGMENT_SIZE)
        flush_log_segment();
    else
    {
        LOG_SEGMENT_ARR[LOG_SEG_IDX].data_size = data_size;
        LOG_SEGMENT_ARR[LOG_SEG_IDX].vdisk_index = db_idx;

        // copy dblock
        for(int i = 0; i<data_size; i++)
            LOG_SEGMENT_ARR[LOG_SEG_IDX].db[i] = db[i];

        // complete block
        for(int i = data_size; i<VDISK_BLOCK_SIZE_BYTES; i++)
            LOG_SEGMENT_ARR[LOG_SEG_IDX].db[i] = '\0';
        
        LOG_SEG_IDX++;
    }
    
    // printLogSeg();
}

inode setup_file_and_update_parent(int is_dir, char* file_path_str)
{
    /* set up new file inode */
    inode new_inode = create_empty_inode(is_dir);
    new_inode.inode_db_idx = get_next_free_block();
    unset_nth_bit(new_inode.inode_db_idx);

    /* initialize new_dir stuff */
    dir_entry new_dir;

    char path_str_cpy[100];

    int last_slash = 0;
    for(new_dir.filename_size = 0; file_path_str[new_dir.filename_size] != '\0'; new_dir.filename_size++)
        if(file_path_str[new_dir.filename_size] == '/')
            last_slash = new_dir.filename_size;

    int idx = 0;
    for(int i = last_slash + 1; i<new_dir.filename_size; i++)
        new_dir.file_name[idx++] = file_path_str[i];
    
    new_dir.file_name[idx] = '\0';
    new_dir.filename_size = idx;

    new_dir.inode_num.i_num = new_inode.inode_db_idx;
    convert_direntry_to_str(&new_dir, 1);
    /* end new_dir stuff */

    /* append to parent dir' dblock */
    int parent_inode_idx = ROOT_INODE_INDEX; // always start at root
    inode parent_inode = walk_down_dir(parent_inode_idx, file_path_str);

    char parent_dir_block[512];
    int parent_inode_ptr = (parent_inode.size * DIRECTORY_ENTRY_SIZE)/VDISK_BLOCK_SIZE_BYTES;
    read_from_segment_or_disk(parent_inode.dblocks_idx[parent_inode_ptr].i_num, parent_dir_block);

    int parent_append_idx = parent_inode.size * DIRECTORY_ENTRY_SIZE;
    for(int i = 0; i<DIRECTORY_ENTRY_SIZE; i++)
        parent_dir_block[parent_append_idx++] = new_dir.entry[i];
    /* end append*/

    parent_inode.size++;

    /* write back parent */
    convert_inode_to_dblock(&parent_inode);
    append_log_segment(parent_inode.inode_db_idx, parent_inode.inode_str, 32);
    append_log_segment(parent_inode.dblocks_idx[parent_inode_ptr].i_num, parent_dir_block, (DIRECTORY_ENTRY_SIZE * parent_inode.size)%VDISK_BLOCK_SIZE_BYTES);
    /* end */

    return new_inode;
}

void create_file(char* file_path_str, char* file_data_str) //TODO parent_what???
{
    inode new_file_inode;
    new_file_inode = setup_file_and_update_parent(0, file_path_str);

    /* write file_block */
    while(file_data_str[new_file_inode.size++] != '\0');

    char new_file_db[VDISK_BLOCK_SIZE_BYTES];
    
    int db_idx = 0;
    int i;
    for(i = 0; i<new_file_inode.size; i++)
    {
        if (i % VDISK_BLOCK_SIZE_BYTES == 0)
        {
            new_file_inode.dblocks_idx[i/VDISK_BLOCK_SIZE_BYTES].i_num = get_next_free_block();
            unset_nth_bit(new_file_inode.dblocks_idx[i/VDISK_BLOCK_SIZE_BYTES].i_num);
            db_idx = 0;

            if(i != 0)
                // write last block
                append_log_segment(new_file_inode.dblocks_idx[i/VDISK_BLOCK_SIZE_BYTES - 1].i_num, new_file_db, VDISK_BLOCK_SIZE_BYTES);
            
            strcpy(new_file_db, "");
        }
        new_file_db[db_idx++] = file_data_str[i];
    }

    // remaining
    if (strcmp(new_file_db, "") != 0)
        append_log_segment(new_file_inode.dblocks_idx[i/VDISK_BLOCK_SIZE_BYTES].i_num, new_file_db, new_file_inode.size % VDISK_BLOCK_SIZE_BYTES);


    /* write file inode */
    convert_inode_to_dblock(&new_file_inode);
    append_log_segment(new_file_inode.inode_db_idx, new_file_inode.inode_str, 32);

    // printf("%d, %s\n", new_file_inode.inode_db_idx, file_path_str);

}

void del_file(char* file_path)
{
    inode parent_inode = walk_down_dir(ROOT_INODE_INDEX, file_path);

    char parent_db[VDISK_BLOCK_SIZE_BYTES];
    char* fname;

    /* file name */
    int fpath_size = 0;
    for(fpath_size; file_path[fpath_size] != '\0'; fpath_size++){}

    int ls;
    for(ls = fpath_size - 1; file_path[ls] != '/'; )
        ls--;

    fname = &file_path[--ls];
    /* end */

    for(int i = 0; parent_inode.dblocks_idx[i].i_num != 0; i++)
    {
        read_from_segment_or_disk(parent_inode.dblocks_idx[i].i_num, parent_db);

        int num_dir_entries;
        int del_idx = -1;
        if(parent_inode.size < VDISK_BLOCK_SIZE_BYTES/DIRECTORY_ENTRY_SIZE)
            num_dir_entries = parent_inode.size;
        else
            num_dir_entries = VDISK_BLOCK_SIZE_BYTES/DIRECTORY_ENTRY_SIZE; // max possible # dir par block
        
        dir_entry parent_dir_arr[num_dir_entries];
        parse_dir_from_db(parent_dir_arr, parent_db, num_dir_entries);

        /* remove dir_entry */
        for(int j = 0; j<num_dir_entries; j++)
            if(strcmp(parent_dir_arr[j].file_name, fname) == 0)
                del_idx = j;

        // anything to del?
        if (del_idx != -1 && del_idx<num_dir_entries)
        {   
            /* free free-block vector */
            inode del_file_inode;
            read_from_segment_or_disk(parent_dir_arr[del_idx].inode_num.i_num, del_file_inode.inode_str);
            parse_inode(&del_file_inode, parent_dir_arr[del_idx].inode_num.i_num);

            for(int s = 0; del_file_inode.dblocks_idx[s].i_num != 0; s++)
                set_nth_bit(del_file_inode.dblocks_idx[s].i_num);

            // free the block
            set_nth_bit(parent_dir_arr[del_idx].inode_num.i_num);

            /* end free free-block vector */

            // remove dir_arr entry
            for(int j = del_idx; j<num_dir_entries - 1; j++)
                parent_dir_arr[j] = parent_dir_arr[j+1];

            num_dir_entries--;
            parent_inode.size--;

            // write back parent inode
            convert_inode_to_dblock(&parent_inode);
            append_log_segment(parent_inode.inode_db_idx, parent_inode.inode_str, DIRECTORY_ENTRY_SIZE * parent_inode.size);

            // write back parent dblock
            convert_direntry_to_str(parent_dir_arr, num_dir_entries);
            int parent_db_idx = 0;

            /* dir_arr -> dblock */
            for(int k = 0; k<num_dir_entries; k++)
                for(int l = 0; parent_dir_arr[k].entry[l] != '\0'; l++)
                    parent_db[parent_db_idx++] = parent_dir_arr[k].entry[l];
            /* end */

            append_log_segment(parent_inode.dblocks_idx[i].i_num, parent_db, parent_inode.size * DIRECTORY_ENTRY_SIZE);
        }
        /* end remove dir_entry */

        // already deleted dir. entry
        if (del_idx != -1)
            break;

    }
}

void mk_dir(char* path_str)
{
    // TODO deal with duplicates
    inode new_dir_inode;

    new_dir_inode = setup_file_and_update_parent(1, path_str);

    /* write dir dblock */
    new_dir_inode.dblocks_idx[0].i_num = get_next_free_block(); // tell dir_inode where the block is at
    unset_nth_bit(new_dir_inode.dblocks_idx[0].i_num);
    convert_int_to_binary_char(new_dir_inode.dblocks_idx[0].i_num, new_dir_inode.dblocks_idx[0].b_num);

    new_dir_inode.size = 0;

    char db[VDISK_BLOCK_SIZE_BYTES] = "";
    append_log_segment(new_dir_inode.dblocks_idx[0].i_num, db, 0);

    // write directory's inode
    convert_inode_to_dblock(&new_dir_inode);
    append_log_segment(new_dir_inode.inode_db_idx, new_dir_inode.inode_str, 32);

    // printf("%d, %s\n", new_dir_inode.inode_db_idx, path_str);

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
    mk_dir("/lulz");
    mk_dir("/lulz2");
    mk_dir("/usr/hello");
    mk_dir("/usr/lol");
    // printf("-------------------------------------------------------\n");

    char *str = "These are reserved words in C language are int, float, "
               "if, else, for, while etc. An Identifier is a sequence of"
               "letters and digits, but must start with a letter. "
               "Underscore ( _ ) is treated as a letter. Identifiers are "
               "case sensitive. Identifiers are used to name variables,"
               "functions etc.These are reserved words in C language are int, float, "
               "if, else, for, while etc. An Identifier is a sequence of"
               "letters and digits, but must start with a letter. "
               "Underscore ( _ ) is treated as a letter. Identifiers are "
               "case sensitive. Identifiers are used to name variables,"
               "functions etc.These are reserved words in C language are int, float, "
               "if, else, for, while etc. An Identifier is a sequence of"
               "letters and digits, but must start with a letter. "
               "Underscore ( _ ) is treated as a letter. Identifiers are "
               "case sensitive. Identifiers are used to name variables,"
               "functions etc.";

    create_file("/usr/hello2/file1", "small file 1");
    create_file("/usr/hello2/file2", "small file 2");

    del_file("/usr/hello2/file1");



    flush_log_segment(); // always flush log segment before terminating
    return 0;
}