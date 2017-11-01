#ifndef ARGPARSE_H
#define ARGPARSE_H


typedef struct argparse_struct * argparse_t;

/** Initialize the parser by giving an array of char * and its length
 *  const char *argv[] an array of char typically the one from the main function
 *  int argc : the length of argv.
 *
 *  return argparse_t, needed by the other functions
 */
argparse_t argparse_init(int argc, const char *argv[]);


/** retrieve an optionnal argument in the form of "-o option_value"
 *  if opt is 'o', then the return value would be "option_value"
 *  the return value is 0 when the argument was not found.
 */
const char * argparse_get_opt(argparse_t parser, char opt);


/** a positional argument is one that does not start with '-' or "--"
 *  this funtion allows to get a positional argument by its position..
 */
const char * argparse_get_positional(argparse_t parser, int arg_num);


/** return 1 if an option in the form of "--my_option" is present
 *  return 0 if it is not present.
 */
int argparse_get_long_opt(argparse_t parser, const char * option);



#endif
