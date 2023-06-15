#include "args.h"

#include <iostream>
#include <vector>
#include <string>
#include <format>

void print_usage(char const* exe_name);

int main(int argc, char** argv) {
    auto args = parse_args(argc, argv);

#ifndef NDEBUG
    debug_print(std::cout, args);
#endif

    if (!args.error.empty()) {
        std::cout << "ERROR: " << args.error << '\n';
        print_usage(argv[0]);
        std::exit(EXIT_FAILURE);
    }

    if (args.hide) {
        std::cout << "hide not yet implemented\n";
    } else if (args.extract) {
        std::cout << "extract not yet implemented\n";
    } else {
        std::cout << "you shouldn't be here!\n";
        std::exit(EXIT_FAILURE);
    }
}

void print_usage(char const* exe_name) {
    std::cout << "Usage:\n";
    std::cout << "    " << exe_name << " --hide -m <message file> -c <coverfile> [-o <stego file>]\n";
    std::cout << "    " << exe_name << " --extract -s <stego file> [-o <message file]\n";
}
