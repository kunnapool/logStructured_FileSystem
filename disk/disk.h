#ifndef DISK_H
#define DISK_H

void write_block_to_disk(int write_location, char* write_block, int data_size);

void read_blocks_from_disk(int num_blocks_to_read, int* blocks_idx_array, char* read_block_ptr);

#endif