
#include <stdint.h>
#include "argparse.h"
#include <stdlib.h>


struct argparse_struct{
    int argc;
    const char ** argv;
};

argparse_t argparse_init(int argc, const char *argv[]){
    argparse_t new_parser = malloc(sizeof(struct argparse_struct));
    new_parser->argc = argc;
    new_parser->argv = argv;
    return new_parser;
}


const char * argparse_get_opt(argparse_t parser, char opt){
    for(int i = 0; i < parser->argc - 1; i++){
        if(parser->argv[i][0] == '-'){
            if(parser->argv[i][1] == opt){
                return parser->argv[i + 1];
            }
        }
    }
    return (char*)0;
}


const char * argparse_get_positional(argparse_t parser, int arg_num){
    for(int i = 1; i < parser->argc; i++){
        if(parser->argv[i][0] != '-'){
            if (arg_num <= 0){
                return parser->argv[i];
            }
            arg_num--;
        }
        else{
          // next argument is an option so we skip it:
          i++;
        }
    }
    return (char*)0;
}
