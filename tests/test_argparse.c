#include <CppUTest/TestHarness.h>

extern "C"{
    #include "argparse.h"
}


TEST_GROUP(test_argparse){};

TEST(test_argparse, argparse_initialisation){
    const char * test_argv[] = {"-t", "test",
                          "-b", "boot.bin"};
    int test_argc = 4;

    argparse_t parser = argparse_init(test_argc, test_argv);
    CHECK_TEXT(parser != 0, "Error in argparse initialisation");
}

TEST(test_argparse, argparse_option){
    const char * test_argv[] = {"./myProgram",
                                "-t", "test",
                                "--fullscreen",
                                "-b", "boot.bin"};
    int test_argc = 6;
    argparse_t parser = argparse_init(test_argc, test_argv);
    const char * boot = argparse_get_opt(parser, 'b');
    const char * empty = argparse_get_opt(parser, 'm');
    STRCMP_EQUAL(test_argv[5], boot);
    CHECK_TEXT(empty == 0, "a non existing option was found!!");
}

TEST(test_argparse, argparse_positionnal){
    const char * test_argv[] = {"./myProgram",
                              "-t", "test",
                              "first_positional",
                              "second_positional",
                              "--fullscreen",
                              "-b", "boot.bin",
                              "last_positional_argument"};
    int test_argc = 9;
    argparse_t parser = argparse_init(test_argc, test_argv);
    const char * pos_0 = argparse_get_positional(parser, 0);
    const char * pos_1 = argparse_get_positional(parser, 2);
    const char * pos_3 = argparse_get_positional(parser, 4);
    STRCMP_EQUAL(test_argv[3], pos_0);
    STRCMP_EQUAL(test_argv[8], pos_1);
    CHECK_TEXT(pos_3 == 0, "a non existing positional argument was found!!");
}


TEST(test_argparse, argparse_long_option){
  const char * test_argv[] = {"./myProgram",
                              "-t", "test",
                              "first_positional",
                              "--skip_frame",
                              "second_positional",
                              "--fullscreen",
                              "-b", "boot.bin",
                              "last_positional_argument"};
    int test_argc = 10;
    argparse_t parser = argparse_init(test_argc, test_argv);
    bool fullscreen = argparse_get_long_opt(parser, "fullscreen") > 0;
    bool reboot = argparse_get_long_opt(parser, "reboot") > 0;
    bool skip = argparse_get_long_opt(parser, "skip") > 0;
    CHECK_TEXT(fullscreen, "option should be present!");
    CHECK_TEXT(!skip, "option should not be present!");
    CHECK_TEXT(!reboot, "option should not be present!");
}
