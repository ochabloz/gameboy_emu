#ifndef UTILS_H
#define UTILS_H

#ifdef __unix__  
#define PATH_SEPARATOR '/'
#else
#define PATH_SEPARATOR '\\'
#endif

char * filename_replace_ext(char * filename, const char * new_ext);
const char * path_get_filename(const char * path);
char * path_concatenate(const char * path, const char * another_path);

#endif // UTILS_H
