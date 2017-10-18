#include <CppUTest/TestHarness.h>

extern "C"{
    #include "argparse.h"
}


TEST_GROUP(test_argparse){};

TEST(test_argparse, argparse_initialisation){
    LONGS_EQUAL(0, 10);
}
