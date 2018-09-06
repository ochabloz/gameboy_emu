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


TEST(test_utils, replace_with_shorter_ext){
    const char * cfile = "somefile.alongext";
    char * rom_file = (char*)malloc(9);
    memcpy(rom_file, cfile, 9);
    rom_file = filename_replace_ext(rom_file, "sav");
    STRCMP_EQUAL("somefile.sav", rom_file);
    free(rom_file);
}

TEST(test_utils, get_filename){
    const char * path = "C:\\Users\\georges\\Downloads\\my-file.ext";
    const char * filename = path_get_filename(path);
    STRCMP_EQUAL("my-file.ext", filename);
}

TEST(test_utils, get_filename_not_found){
    const char * path = "C:\\Users\\georges\\Downloads\\";
    const char * filename = path_get_filename(path);
    CHECK(filename == NULL);
}

TEST(test_utils, is_filename){
    const char * path = "my-file.ext";
    const char * filename = path_get_filename(path);
    STRCMP_EQUAL("my-file.ext", filename);
}

TEST(test_utils, path_concatenate){
    const char * path1 = "C:\\Users\\georges\\Downloads\\";
    const char * path11 = "C:\\Users\\georges\\Downloads";
    const char * path2 = "my-file.ext";
    const char * filename = path_concatenate(path1, path2);
    STRCMP_EQUAL("C:\\Users\\georges\\Downloads\\my-file.ext", filename);
    const char * filename2 = path_concatenate(path11, path2);
    STRCMP_EQUAL("C:\\Users\\georges\\Downloads\\my-file.ext", filename2);
}