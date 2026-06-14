#ifndef SEARCH_FILE_H
#define SEARCH_FILE_H

#define MAX_TARGET_PATH 60
#define UNINITIALIZED -125
#define PERMISION_SIZE 10

#define _XOPEN_SOURCE 600 
#include <ftw.h>

typedef struct{

    char file_name[20];
    long int file_size;
    char file_type;                                        
    char file_permision[10];
    int file_number_of_links;

} file_info;



int function_for_nftw(const char *pathname, const struct stat *buf, int type, struct FTW *ftwb); /*The function which is parameter of nftw function.*/
int file_name_comparison(char *file_name);
void get_command(int argc, char *argv[], char *target_path);
void fill_file_info(file_info *fl, const  char *pathname, const struct stat *buf, int type, struct FTW *ftwb);
char checking_of_file_type(mode_t type);
void initialized_file_info();
void print();
int file_comparison(file_info *fl);
void convert_permisions_to_string(char *string, mode_t type);
int command_extract_for_digit(char *command);
char command_extract_for_alpha(char *command);



#endif
