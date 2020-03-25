#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<string.h>

static const int VDISK_BLOCK_SIZE_BYTES = 512;
static const char VDISK_PATH_STR[] = "vdisk";

void write_block_to_disk(int write_location, char* write_block, int data_size)
{
    FILE* vdisk_ptr = fopen(VDISK_PATH_STR, "r+");

    fseek(vdisk_ptr, write_location * VDISK_BLOCK_SIZE_BYTES, SEEK_SET);
    fwrite(write_block, sizeof(char), data_size, vdisk_ptr);

    // fill up remaining space with zeros
    for(int i = 0; i<VDISK_BLOCK_SIZE_BYTES - data_size; i++)
        fputc(0, vdisk_ptr);

    fclose(vdisk_ptr);
}

void read_blocks_from_disk(int num_blocks_to_read, int* blocks_idx_array, char* read_block_ptr)
{
    FILE* vdisk_ptr = fopen(VDISK_PATH_STR, "r");

    for(int i=0;i <num_blocks_to_read; i++)
    {
        char temp[VDISK_BLOCK_SIZE_BYTES+1];

        fseek(vdisk_ptr, blocks_idx_array[i] * VDISK_BLOCK_SIZE_BYTES, SEEK_SET); // offset (blocks) from start of file
        fread(temp, sizeof(char), VDISK_BLOCK_SIZE_BYTES, vdisk_ptr); //read one block

        // append each byte to output
        for(int j=0; j<VDISK_BLOCK_SIZE_BYTES; j++)
            read_block_ptr[i*VDISK_BLOCK_SIZE_BYTES + j] = temp[j];
    }
    fclose(vdisk_ptr);
}