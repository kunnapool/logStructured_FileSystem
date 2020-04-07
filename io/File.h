#ifndef FFS_H
#define FFS_H

void initLLFS();

void mk_dir(char* path_str);

void create_file(char* file_path_str, char* file_data_str);

void del_file(char* file_path);

void flush_log_segment();

#endif