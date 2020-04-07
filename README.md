# csc360_Ass3

Simple Log based file system. The design here, for the purposes of this assignment, is
intentionally kept simple. The first block is the superblock. The first 4 bytes of the superblock
are the magic number followed by # blocks and # inodes. Next is free bit vector. As is convention,
the root inode is at a fixed location, block index 2.

Being a LFS, this file system writes blocks to an in-memory segment and only writes to
disk when the segment is full (segment size is kept at 10 blocks). That being said, for the test cases,
a call to flush_log_segment is made at the end in order to explicitly flush out all the blocks.

The test cases all start by "mounting" the FS by calling initLLFS(), which sets up the superblock,
free-bit vector and the root inode.

To create directories, a function mk_dir is used. Directories can only be created one level at a time.
Files are created by create_file(file_path, file_str) - the can be more than one block. To update files in
this FS, the file must be deleted by del_file and created again - other forms of "updating" is not supported.

For inodes, the first 4 bytes store the size in a 4 byte num-char, and the next 4 indicate whether
the underlying file is a directory. For file data-block-pointers, the block numbers are stored as
bit-modified 2 byte chars - NOTE: this does not show up using hexdump since the encoding is on a bit-level,
to verify this, "xxd -b -c 8 vdisk" can be used that displays individual bits. Futhermore, instead of a inode map,
as a tradeoff with simplicity, the changes are made to blocks' pointers.

For directory entries, the first two bytes are bit-modified chars, and the next 20 bytes are the directory
name. The total size is 22 bytes. Creating a directory using mk_dir creates an inode for the directory along
with an empty data-block.

Due to the time crunch, robustness was not included in this FS. The plan: an fsck like check that checks
whether or not the free-bit vector agrees with the actual data present.