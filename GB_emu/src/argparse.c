
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
        else if(parser->argv[i][1] != '-'){
          // next argument is an option so we skip it:
          i++;
        }
    }
    return (char*)0;
}


int argparse_get_long_opt(argparse_t parser, const char * option){
    for (int i = 0; i < parser->argc; i++) {
        if(parser->argv[i][0] == '-'){
            if(parser->argv[i][1] == '-'){
                int is_match = 1;
                unsigned int pos = 2;
                while (is_match) {
                    if(parser->argv[i][pos] == '\0' && option[pos -2] == '\0'){
                        break; // end of comparaison, both string are identical
                    }
                    else if(parser->argv[i][pos] == '\0' || option[pos -2] == '\0'){
                        // one of the str is shorter. thus not identical
                        is_match = 0;
                    }
                    if (parser->argv[i][pos] == option[pos -2]) {
                        pos++;
                    }
                    else{
                        is_match = 0;
                    }
                }
                if(is_match){
                    return 1;
                }
            }
        }
    }
    return 0;
}
