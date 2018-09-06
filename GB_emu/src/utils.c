#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

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


const char * path_get_filename(const char * path) {
	size_t len = strlen(path);
	if (!(path[len - 1] == PATH_SEPARATOR)) {
		for (int i = len; i > 1; i--) {
			if (path[i - 1] == PATH_SEPARATOR) {
				return path + i;
			}
		}
		// nothing was found. path must be a filename..
		return path;
	}
	return NULL;
}

char * path_concatenate(const char * path, const char * another_path){
    size_t len = strlen(path);
    int has_separator = (path[len -1] == PATH_SEPARATOR);
    size_t len_total = (has_separator) ? len : len + 1;
    len_total += strlen(another_path);
    char * ret_str = (char*)malloc(len_total + 1);
	strcpy(ret_str, path);
    if(has_separator){
        strcat(ret_str, another_path);
    }
    else{
		ret_str[len] = PATH_SEPARATOR;
        strcat(ret_str, another_path);
    }
    return ret_str;
}