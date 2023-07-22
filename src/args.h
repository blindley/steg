#ifndef ARGS_202306151743
#define ARGS_202306151743

#include <string>

#include "utility.h"

struct Args {
    bool help;
    bool extract;
    bool hide;
    bool measure;
    int random_count;
    std::string message_file;
    std::string cover_file;
    std::string stego_file;
    std::string output_file;
    float threshold;
    u8 rmax;
    u8 gmax;
    u8 bmax;
    u8 amax;
};

Args parse_args(int argc, char** argv);
void print_usage(char const* exe_name);
void print_help(char const* exe_name);

#endif // ARGS_202306151743
