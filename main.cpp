#include "args.h"

#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <format>
#include <filesystem>
#include <unordered_map>
#include <algorithm>

void print_usage(char const* exe_name);

int main(int argc, char** argv) {
    auto args = parse_args(argc, argv);

    debug_print(std::cout, args);

    if (!args.error.empty()) {
        std::cout << "ERROR: " << args.error << '\n';
        print_usage(argv[0]);
        std::exit(EXIT_FAILURE);
    }
}

void print_usage(char const* exe_name) {
    std::cout << "Usage:\n";
    std::cout << "    " << exe_name << " --hide -m <message file> -c <coverfile> [-o <stego file>]\n";
    std::cout << "    " << exe_name << " --extract -s <stego file> [-o <message file]\n";
}
