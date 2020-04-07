#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<string.h>
#include "../disk/disk.h"

#define VDISK_BLOCK_SIZE_BYTES 512
static const int MAX_BLOCKS = 4096;
static const int MAX_INODES = 128;

static const char VDISK_PATH_STR[] = "vdisk";

void convert_int_to_binary_char(int x, char* binary_num)
{
    int temp = x;
    char bitmask_array[] = {0b10000000, 0b01000000, 0b00100000, 0b00010000, 0b00001000, 0b00000100, 0b00000010, 0b00000001};
    binary_num[0] = 0b00000000;
    binary_num[1] = 0b00000000;

    int i = 0;
    int idx = 1;

    // extract binary
    while(temp != 0)
    {

        if (i == 8)
        {
            i = 0;
            idx--;
            continue;
        }
        if (temp % 2 == 1)
            binary_num[idx] = binary_num[idx] | bitmask_array[7 - i];
        
        i++;
        temp /= 2;
        
    }

    // binary_num[2] = '\0';

}

int convert_binary_char_to_int(char* binary_num)
{
    int power = 15;
    int int_num = 0;
    char bitmask_array[] = {0b10000000, 0b01000000, 0b00100000, 0b00010000, 0b00001000, 0b00000100, 0b00000010, 0b00000001};

    for(int idx = 0; idx <= 1; idx++)
    {
        for(int i = 0; i<8; i++)
        {
            if((binary_num[idx] & bitmask_array[i]) != 0)
                int_num += pow(2, power);
            
            power--;
        }
    }

    return int_num;
}

void setup_vdisk()
{
    FILE* vdisk_ptr = fopen(VDISK_PATH_STR, "w");
    fclose(vdisk_ptr);

    char super_block[20] = "";
    char magic_no[5] = "bruh";
    char num_blocks[5] = "";
    char inodes[5] = "";

    sprintf(num_blocks, "%d", MAX_BLOCKS);
    sprintf(inodes, "0%d", MAX_INODES);
    
    strcat(super_block, magic_no);
    strcat(super_block, num_blocks);
    strcat(super_block, inodes);

    // printf("%s", super_block);

    write_block_to_disk(0, super_block, 12);

}

void setup_free_block()
{
    FILE* vdisk_ptr = fopen(VDISK_PATH_STR, "r+");

    char all_ones = 0b11111111;
    char all_zeros = 0b00000000;
    char ten_zeros[] = {0b00000000, 0b00111111};
    
    char fb[512];
    fb[0] = ten_zeros[0];
    fb[1] = ten_zeros[1];

    for(int i = 2; i<VDISK_BLOCK_SIZE_BYTES; i++)
        fb[i] = all_ones;

    write_block_to_disk(1, fb, VDISK_BLOCK_SIZE_BYTES);
    
}

void initLLFS()
{
    setup_vdisk();
    setup_free_block();
}

void set_nth_bit(int n)
{
    char bitmask_array[] = {0b10000000, 0b01000000, 0b00100000, 0b00010000, 0b00001000, 0b00000100, 0b00000010, 0b00000001};

    int x = 1;
    char block[VDISK_BLOCK_SIZE_BYTES];
    
    read_blocks_from_disk(1, x, block);

    int row = n/8;
    int offset = n%8;

    char c = block[row];

    block[row] = block[row] | bitmask_array[offset];

    write_block_to_disk(1, block, 512);

}

void unset_nth_bit(int n)
{
    char bitmask_array[] = {0b01111111, 0b10111111, 0b11011111, 0b11101111, 0b011110111, 0b11111011, 0b11111101, 0b11111110};

    int x = 1;
    char block[VDISK_BLOCK_SIZE_BYTES];
    
    read_blocks_from_disk(1, x, block);

    int row = n/8;
    int offset = n%8;

    // printf("---%d, %d, %d---\n", n, row, offset);
    block[row] = block[row] & bitmask_array[offset];

    write_block_to_disk(1, block, 512);

}

void count_all_set_bits()
{
    char bitmask_array[] = {0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00010000, 0b00100000, 0b01000000, 0b10000000};

    int free_bit_block_idx = 1;

    char block[VDISK_BLOCK_SIZE_BYTES];

    read_blocks_from_disk(1, free_bit_block_idx, block);

    int count = 0;
    for(int i=0; i<VDISK_BLOCK_SIZE_BYTES; i++)
    {
        char c = block[i];

        for(int j=0; j<8; j++)
            if (c & bitmask_array[j])
                count++;
    }


}

int get_next_free_block()
{
    char free_block[512];
    int fb_idx = 1;

    char bitmask_array[] = {0b10000000, 0b01000000, 0b00100000, 0b00010000, 0b00001000, 0b00000100, 0b00000010, 0b00000001};

    read_blocks_from_disk(1, fb_idx, free_block);

    fb_idx = 0; // free data-blocks start at 10
    for(int i = 0; i<VDISK_BLOCK_SIZE_BYTES; i++)
    {
        char c = free_block[i];

        for(int j = 0; j<8; j++)
        {
            if((c & bitmask_array[j]) != 0)
                break;

            fb_idx++;
        }

    }

    return fb_idx;
}