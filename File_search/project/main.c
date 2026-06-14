#include "search_file.h"
#include <stdio.h>
#include <stdlib.h>

extern int FLAG_FILE;

int main(int argc, char *argv[]) {

    char target_path[MAX_TARGET_PATH];
    int flags = 0;
    int status;

    flags |= FTW_CHDIR;
    

    get_command(argc, argv, target_path);
    status = nftw(target_path, function_for_nftw, 10, flags);

    if(status == -1) {
        perror("nft function error");
        exit(EXIT_FAILURE);
    }

    if(FLAG_FILE == 0) {
        printf("\nNot found.\n");
    }

   /* char target_path[MAX_TARGET_PATH];
    char string[10];
    struct stat buf;
    char type;
    int flag;

    get_command(argc, argv, target_path);
    printf("target path = %s\n", target_path);
    print();

    stat(".", &buf);

    convert_permisions_to_string(string, (buf.st_mode));

    printf("permition : %s\n", string);
    type = checking_of_file_type(buf.st_mode);

    printf("type = %c\n", type);

    flag = file_name_comparison(argv[argc - 1]);

    printf("istrue ? %d\n", flag);

    */

    exit(EXIT_SUCCESS);
    
}