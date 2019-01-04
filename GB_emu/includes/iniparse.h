#ifndef INIPARSE_H
#define INIPARSE_H


typedef struct iniparse_struct * iniparse_t;


iniparse_t iniparse_init(const char * filename);
char * iniparse_get_section(const char * line);
char * iniparse_get_value(const char * line, char ** value);
int iniparse_get_bool(iniparse_t parser, const char * key);
const char * iniparse_get_key(iniparse_t parser, const char * key);
int iniparse_get_int(iniparse_t parser, const char * key);

#endif