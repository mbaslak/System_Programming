#include "search_file.h"


#include <ftw.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

extern int optind, opterr, optopt;
extern char *optarg;

static file_info command;
int FLAG_FILE = 0;


void get_command(int argc, char *argv[], char *target_path) {

    int opt;
    int flag_w = 0;
    int flag_f = 0;
    int flag_b = 0;
    int flag_t = 0;
    int flag_p = 0;

    int flag_l = 0;

    initialized_file_info();

    while((opt = getopt(argc, argv, ":w:f:b:t:l:p:x")) != -1) {

        switch(opt) {
            case 'w': 
                    if(flag_w == 1) {
                        fprintf(stderr, "You should use -w flag once\n");
                    }
                    else {
                        flag_w = 1;
                        strcpy(target_path, optarg);
                    }break;
            case 'f':
                    if(flag_f == 1) {
                        fprintf(stderr, "You should use -f flag once\n");
                    }
                    else {
                        flag_f = 1;
                        strcpy(command.file_name, optarg);
                    }break;
            case 'b':
                    if(flag_b == 1) {
                        fprintf(stderr, "You should use -b flag once\n");
                    }
                    else {
                        flag_b = 1;
                        command.file_size = command_extract_for_digit(optarg);
                    }break;
            case 't':
                    if(flag_t == 1) {
                        fprintf(stderr, "You should use -t flag once\n");
                    }
                    else {
                        flag_t = 1;
                        command.file_type = command_extract_for_alpha(optarg);
                    }break;
            case 'p':
                    if(flag_p == 1) {
                        fprintf(stderr, "You should use -p flag once\n");
                    }
                    else {
                        flag_p = 1;
                        strcpy(command.file_permision, optarg);
                    }break;
            case 'l':
                    if(flag_l == 1) {
                        fprintf(stderr, "You should use -l flag once\n");
                    }
                    else {
                        flag_l = 1;
                        command.file_number_of_links = command_extract_for_digit(optarg);
                    }break;
            case '?':
                    fprintf(stderr, "Invalid argument.\n");
                    exit(EXIT_FAILURE); break;
            case ':':
                    fprintf(stderr, "Missing argument.\n");
                    exit(EXIT_FAILURE); break;
            
            default:
                    fprintf(stderr, "Unacceptable flag.\n");
                    exit(EXIT_FAILURE); break;
        }
    }

    if(flag_w == 0) {
        fprintf(stderr, "You should use -w flag.\n");
        exit(EXIT_FAILURE);
    }

}

void initialized_file_info() {

    strcpy(command.file_name, "");
    strcpy(command.file_permision, "");
    command.file_number_of_links = UNINITIALIZED;
    command.file_size = UNINITIALIZED;
    command.file_type = '*';

}

int command_extract_for_digit(char *command) {

    int status;
    long int number;
    int flag = 0;
    while(!flag) {
        status = sscanf(command, "%ld", &number);
        if(status != 1) {
            fprintf(stderr, "Please, enter a integer number\n");
            exit(EXIT_FAILURE);
        }
        else flag = 1;
    }

    return number;
}

char command_extract_for_alpha(char *command) {
    return command[0];
}

/*void correction_file_name(char *name, char *arg) {

    int length = strlen(arg);

    if((arg[0] != '\'' || arg[length - 1] != '\'') && (arg[0] != '`' || arg[length - 1] != '`')) {
        fprintf(stderr, "Please, enter file name correct format:\n'filename' or `filename`\n");
        exit(EXIT_FAILURE);
    }

    strcpy(name, arg + 1);
    name[length - 2] = '\0';
}*/



int function_for_nftw(const char *pathname, const struct stat *buf, int type, struct FTW *ftwb) {

    /*printf("error 4\n");*/

    file_info *fl;
    int flag = 0;
    int index;

    fl = (file_info*)malloc(sizeof(file_info));

    fill_file_info(fl, pathname, buf, type, ftwb);

    flag = file_comparison(fl);

  
    if(ftwb->level != 0) printf("|");
    for(index = 0; index < ftwb->level; index++) printf("---");
    
    if(flag == 1) {
        printf("\33[1m%s\n\33[0m", &pathname[ftwb->base]);
        FLAG_FILE = 1;
    }
    else printf("%s\n", &pathname[ftwb->base]);
    

    free(fl);

    return 0;
}

int file_comparison(file_info *fl) {

    int r = 1;

    if((strcmp(command.file_name, "") != 0) && !file_name_comparison(fl->file_name)) r = 0;
    if((r == 1) && (strcmp(command.file_permision, "") != 0) && (strcmp(command.file_permision, fl->file_permision) != 0)) r = 0;
    if((r == 1) && (command.file_number_of_links != UNINITIALIZED) && command.file_number_of_links != fl->file_number_of_links) r = 0;
    if((r == 1) && (command.file_size != UNINITIALIZED) && command.file_size != fl->file_size) r = 0;
    if((r == 1) && (command.file_type != '*') && command.file_type != fl->file_type) r = 0;
    
    return r;

}

int file_name_comparison(char *file_name) {

    int r;
    int length_command = strlen(command.file_name);
    int length_file = strlen(file_name);
    int index_command = 0, index_file = 0;
    char c_letter;
    char f_letter;
    char c_letter_before;
    int flag = 0;

    while(index_command < length_command && index_file < length_file && !flag) {

        c_letter = tolower(command.file_name[index_command]);
        f_letter = tolower(file_name[index_file]);
        

        if(c_letter == f_letter) {
            index_command++;
            index_file++;
        }
        else if(c_letter == '+' && index_command != 0) {
            c_letter_before = command.file_name[index_command - 1];
            
            if(c_letter_before == f_letter) {
                index_file++;
            }
            else {
                index_command++;
            }
        }
        else flag = 1;
    }

    if(command.file_name[index_command] != '\0' || file_name[index_file] != '\0' || flag == 1) r = 0;
    else r = 1;

    return r;
    
}

void print() {
    printf("%s\n%s\n%c\n%d\n%ld", command.file_name, command.file_permision, command.file_type, command.file_number_of_links, command.file_size);
}

void fill_file_info(file_info *fl, const char *pathname, const struct stat *buf, int type, struct FTW *ftwb) {

    char permision[PERMISION_SIZE];
    char file_type;

    strcpy(fl->file_name, &pathname[ftwb->base]);
    convert_permisions_to_string(permision, buf->st_mode);
    strcpy(fl->file_permision, permision);
    fl->file_number_of_links = buf->st_nlink;
    fl->file_size = buf->st_size;
    file_type = checking_of_file_type(buf->st_mode);
    fl->file_type = file_type;

}


char checking_of_file_type(mode_t type) {

    char r;

    if(S_ISREG(type)) r = 'r';
    else if(S_ISDIR(type)) r = 'd';
    else if(S_ISCHR(type)) r = 'c';
    else if(S_ISBLK(type)) r = 'b';
    else if(S_ISFIFO(type)) r = 'p';
    else if(S_ISSOCK(type)) r = 's';
    else if(S_ISLNK(type)) r = 'l';

    return r;

}

void convert_permisions_to_string(char *string, mode_t type) {

    if(type & S_IRUSR) strcat(string, "r");
    else strcat(string, "-");

    if(type & S_IWUSR) strcat(string, "w");
    else strcat(string, "-");

    if(type & S_IXUSR) strcat(string, "x");
    else strcat(string, "-");

    if(type & S_IRGRP) strcat(string, "r");
    else strcat(string, "-");

    if(type & S_IWGRP) strcat(string, "w");
    else strcat(string, "-");

    if(type & S_IXGRP) strcat(string, "x");
    else strcat(string, "-");

    if(type & S_IROTH) strcat(string, "r");
    else strcat(string, "-");

    if(type & S_IWOTH) strcat(string, "w");
    else strcat(string, "-");

    if(type & S_IXOTH) strcat(string, "x");
    else strcat(string, "-");

}

