#ifndef ARGS_202306151743
#define ARGS_202306151743

#include <string>
#include <ostream>

#include "utility.h"

struct Args {
    bool help;
    bool extract;
    bool hide;
    bool measure;
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

void debug_print(std::ostream& ostr, Args const& args);
Args parse_args(int argc, char** argv);

#endif // ARGS_202306151743
