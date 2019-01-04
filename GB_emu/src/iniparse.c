#include <stdint.h>
#include "iniparse.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


struct iniparse_struct{
    char * key;
    char * value;
    iniparse_t next;
};

iniparse_t new_iniparse(char * key, char * value){
    iniparse_t new_ini = malloc(sizeof(struct iniparse_struct));
    new_ini->key = key;
    new_ini->value = value;
    new_ini->next = NULL;
    return new_ini;
}

char * trim_to_new_str(const char * line, int start_pos, int stop_pos){
    while(line[start_pos] == ' ' && stop_pos > start_pos) start_pos++;
    while((line[stop_pos-1] <= ' ') && stop_pos > start_pos) stop_pos--;
    size_t trim_len = (stop_pos - start_pos + 1);
    char * trim = malloc(sizeof(char) * trim_len);
    memset(trim, '\0', trim_len);
    memcpy(trim, line + start_pos, trim_len-1);
    return trim;
}

char * iniparse_get_section(const char * line){
    uint32_t i = 0;
    int32_t start_pos = -1, stop_pos = -1;
    while(line[i] != '\n'){
        if(line[i] == '[' && start_pos < 0) start_pos = i + 1;
        if(line[i] == ']' && start_pos > 0 && stop_pos < 0){
            stop_pos = i;
            return trim_to_new_str(line, start_pos, stop_pos);
        }
        i++;
    }
    return NULL;
}

char * iniparse_get_value(const char * line, char ** value){
    size_t len = strlen(line);
    uint32_t i = 0;
    int32_t start_kpos, stop_kpos, start_vpos = -1;
    while((line[i] < 'A' || line[i] > 'z') && i < len && line[i] != ';'){i++;}

    if(i == len || line[i] == ';') return NULL;
    start_kpos = i;

    while(line[i] != '=' && i < len) i++;

    if(i == len) return NULL;
    stop_kpos = i;
    i++;

    start_vpos = i;
    while(line[i] != ';' && i < len) i++;
    if(start_kpos < 0 || stop_kpos < 0 || start_vpos < 0){
        return NULL;
    }
    *value = trim_to_new_str(line, start_vpos, i);
    if(strlen(*value) == 0) return NULL;
    return trim_to_new_str(line, start_kpos, stop_kpos);
    return NULL;
}

iniparse_t iniparse_init(const char * filename){
    iniparse_t ini_parser = NULL;
    iniparse_t * next_ini = &ini_parser;
    FILE * fp = fopen(filename, "r");
    char line[80];
    if(fp < 0){
        return NULL;
    }
    char * actual_section = NULL;
    while(fgets(line, sizeof(line), fp) != NULL){
        char * s = iniparse_get_section(line);
        if(s != NULL){
            if(actual_section != NULL){ 
                free(actual_section);
            }
            actual_section = s;
        }
        else if(actual_section != NULL){
            char * v = NULL;
            char * k = iniparse_get_value(line, &v);
            if(k != NULL){
                char * new_key = malloc(sizeof(char) * (strlen(actual_section) + strlen(k) + 2));
                sprintf(new_key, "%s:%s", actual_section, k);
                free(k);
                *next_ini = new_iniparse(new_key, v);
                next_ini = &(*next_ini)->next;
            }
        }
    }
    return ini_parser;
}


int iniparse_get_bool(iniparse_t parser, const char * key){
    const char * v = iniparse_get_key(parser, key);
    if(v != NULL){
        if(!strcmp("on", v)) return 1;
        if(!strcmp("off", v)) return 0;
        if(!strcmp("ON", v)) return 1;
        if(!strcmp("OFF", v)) return 0;
        if(!strcmp("true", v)) return 1;
        if(!strcmp("false", v)) return 0;
        if(!strcmp("True", v)) return 1;
        if(!strcmp("False", v)) return 0;
        if(!strcmp("TRUE", v)) return 1;
        if(!strcmp("FALSE", v)) return 0;
    }
    return -1;
}

int iniparse_get_int(iniparse_t parser, const char * key){
    const char * v = iniparse_get_key(parser, key);
    if(v != NULL){
        return strtol(v, NULL, 0);
    }
    return -1;
}

const char * iniparse_get_key(iniparse_t parser, const char * key){
    while(1){
        if(!strcmp(key, parser->key)){
            return parser->value;
        }
        if (parser->next != NULL) {
            parser = parser->next;
        }
        else{
            return NULL;
        }
    }
}