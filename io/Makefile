CC := gcc

all: Test

Test: File.c ../disk/disk.h ../disk/disk.c ffs_helper.h ffs_helper.c
	$(CC) -g -o ffs File.c ../disk/disk.c ffs_helper.c -lm
