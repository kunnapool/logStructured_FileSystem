#ifndef FFS_H
#define FFS_H


static const int VDISK_BLOCK_SIZE_BYTES = 512;
static const int MAX_BLOCKS = 4096;
int MAX_VDISK_SIZE_BYTES = VDISK_BLOCK_SIZE_BYTES * MAX_BLOCKS; // 2 MB in Bytes
static const int MAX_INODES = 128;

static const char VDISK_PATH_STR[] = "vdisk";

void convert_int_to_binary_char(int x, char* binary_num);

int convert_binary_char_to_int(char* binary_num);

void setup_vdisk();

void setup_free_block();

void initLLFS();

void set_nth_bit(int n);

void unset_nth_bit(int n);

void count_all_set_bits();

int get_next_free_block();

#endif