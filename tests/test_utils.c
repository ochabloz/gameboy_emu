#include <CppUTest/TestHarness.h>

extern "C"{
    #include "utils.h"
}
#include <stdlib.h>
#include <string.h>


TEST_GROUP(test_utils){};

TEST(test_utils, replace_extention){
    const char * cfile = "somefile.gb";
    char * rom_file = (char*)malloc(12);
    memcpy(rom_file, cfile, 12);
    rom_file = filename_replace_ext(rom_file, "sav");
    STRCMP_EQUAL("somefile.sav", rom_file);

    rom_file = filename_replace_ext(rom_file, "averylongextention");
    STRCMP_EQUAL("somefile.averylongextention", rom_file);
    free(rom_file);
}

TEST(test_utils, replace_with_no_extention){
    const char * cfile = "somefile";
    char * rom_file = (char*)malloc(9);
    memcpy(rom_file, cfile, 9);
    rom_file = filename_replace_ext(rom_file, "sav");
    STRCMP_EQUAL("somefile.sav", rom_file);
    free(rom_file);
}
