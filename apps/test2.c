#include<stdio.h>

#include "../io/File.h"
#include "../disk/disk.h"

int main()
{
    initLLFS();

    mk_dir("/usr");
    mk_dir("/csc360");

    char *str = "These are reserved words in C language are int, float, "
           "if, else, for, while etc. An Identifier is a sequence of"
           "letters and digits, but must start with a letter. "
           "Underscore ( _ ) is treated as a letter. Identifiers are "
           "case sensitive. Identifiers are used to name variables,"
           "functions etc.These are reserved words in C language are int, float, "
           "if, else, for, while etc. An Identifier is a sequence of"
           "letters and digits, but must start with a letter. "
           "Underscore ( _ ) is treated as a letter. Identifiers are "
           "case sensitive. Identifiers are used to name variables,"
           "functions etc.These are reserved words in C language are int, float, "
           "if, else, for, while etc. An Identifier is a sequence of"
           "letters and digits, but must start with a letter. "
           "Underscore ( _ ) is treated as a letter. Identifiers are "
           "case sensitive. Identifiers are used to name variables,"
           "functions etc.";

    create_file("/gibberish", str);


    mk_dir("/csc360/disk");
    mk_dir("/csc360/io");
    mk_dir("/csc360/apps");

    create_file("/csc360/disk/driver.c", "He who drives the disk!!!");
    create_file("/csc360/io/File.c", "Best FS eva!!!");
    create_file("/csc360/apps/test2.c", "Wow, so meta!!!");

    flush_log_segment();

    return 0;
}