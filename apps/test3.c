#include<stdio.h>

#include "../io/File.h"
#include "../disk/disk.h"

int main()
{
    initLLFS();

    mk_dir("/usr");

    create_file("/small_file", "Je suis vraiment petit");

    del_file("/small_file"); // NOTE: this will delete the dir. entry and so won't be visible in root

    flush_log_segment();
    return 0;
}