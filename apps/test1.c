#include<stdio.h>

#include "../io/File.h"
#include "../disk/disk.h"

int main()
{
    initLLFS();

    mk_dir("/usr");
    mk_dir("/local");
    mk_dir("/local/tmp");

    mk_dir("/bin");
    mk_dir("/bin/dev");
    
    mk_dir("/usr/bin");
    mk_dir("/usr/kunal");
    mk_dir("/usr/kunal/csc360");
    mk_dir("/usr/kunal/csc361");
    mk_dir("/usr/kunal/csc111");
    mk_dir("/usr/kunal/csc462");

    flush_log_segment();

    return 0;
}