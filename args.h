#include <string>
#include <ostream>

struct Args {
    bool extract;
    bool hide;
    std::string message_file;
    std::string cover_file;
    std::string stego_file;
    std::string output_file;

    std::string error;
};

void debug_print(std::ostream& ostr, Args const& args);
Args parse_args(int argc, char** argv);
