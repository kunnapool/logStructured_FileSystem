CC := gcc

all: Test1 Test2 Test3

Test1: test1.c ../io/File.c ../io/ffs_helper.h ../io/ffs_helper.c ../disk/disk.h ../disk/disk.c
	$(CC) -g -o test1 test1.c ../io/File.c ../io/ffs_helper.c ../disk/disk.c -lm

Test2: test2.c ../io/File.c ../io/ffs_helper.h ../io/ffs_helper.c ../disk/disk.h ../disk/disk.c
	$(CC) -g -o test2 test2.c ../io/File.c ../io/ffs_helper.c ../disk/disk.c -lm

Test3: test3.c ../io/File.c ../io/ffs_helper.h ../io/ffs_helper.c ../disk/disk.h ../disk/disk.c
	$(CC) -g -o test3 test3.c ../io/File.c ../io/ffs_helper.c ../disk/disk.c -lm