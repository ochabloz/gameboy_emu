#include <CppUTest/TestHarness.h>

extern "C"{
    #include "iniparse.h"
}


TEST_GROUP(test_iniparse){};

TEST(test_iniparse, iniparse_section){
    char * section = iniparse_get_section("   [ASDFvd] ; Whatever");
    STRCMP_EQUAL(section, "ASDFvd");
}

TEST(test_iniparse, iniparse_value){
    char * v = NULL;
    // Testing simple key/value
    char * key = iniparse_get_value("fullscreen=ON", &v);
    STRCMP_EQUAL("fullscreen", key); STRCMP_EQUAL("ON", v);

    // Testing key / value and comment
    key = iniparse_get_value("fullscreen = ON ; un petit poisson", &v);
    STRCMP_EQUAL("fullscreen", key); STRCMP_EQUAL("ON", v);

    key = iniparse_get_value("color0 = 0xdeadb3ef ; un petit poisson", &v);
    STRCMP_EQUAL("color0", key); STRCMP_EQUAL("0xdeadb3ef", v);

    // Testing Comment line
    key = iniparse_get_value("; un petit poisson = un petit oiseau..", &v);
    STRCMP_EQUAL(NULL, key);

    // Testing malformed key/value
    key = iniparse_get_value("fullscreen= ", &v);
    CHECK_TEXT(key == NULL, "Non valid lines must return null");
}

TEST(test_iniparse, iniparse_initialisation){
    iniparse_t ini = iniparse_init("GB.cfg");
    CHECK_TEXT(ini != 0, "iniparser is empty");

    CHECK(iniparse_get_bool(ini, "GB:fullscreen"));
    STRCMP_EQUAL("mybootrom.bin", iniparse_get_key(ini, "GB:bios"));
    LONGS_EQUAL(8962160, iniparse_get_int(ini, "Palette:c1"));
    LONGS_EQUAL(0xe0f8d0, iniparse_get_int(ini, "Palette:c0"));
}