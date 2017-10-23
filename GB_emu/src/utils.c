#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char * filename_replace_ext(char * filename, const char * new_ext){
    int filename_len = 0;
    while (filename[filename_len] != '\0'){filename_len++;}

    int new_ext_len = 0;
    while (new_ext[new_ext_len] != '\0'){new_ext_len++;}

    int dot_pos = filename_len -1;
    while (filename[dot_pos] != '.' && dot_pos > 0){dot_pos--;}
    if(dot_pos == 0){
        dot_pos = filename_len;
        filename[filename_len] = '.';
    }
    if(dot_pos + new_ext_len != filename_len){
        filename = realloc(filename, dot_pos + new_ext_len + 1);
    }
    memcpy(filename + dot_pos + 1, new_ext, new_ext_len);
    filename[dot_pos + new_ext_len + 1] = '\0';
    return filename;
}
