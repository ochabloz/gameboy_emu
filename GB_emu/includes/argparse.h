#ifndef ARGPARSE_H
#define ARGPARSE_H


typedef struct argparse_struct * argparse_t;

argparse_t argparse_init(int argc, const char *argv[]);

const char * argparse_get_opt(argparse_t parser, char opt);
const char * argparse_get_positional(argparse_t parser, int arg_num);
int argparse_get_long_opt(argparse_t parser, const char * option);
#endif
