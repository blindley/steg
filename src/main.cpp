#include "args.h"
#include "image.h"
#include "steg.h"
#include "utility.h"

#include <iostream>
#include <vector>
#include <string>
#include <format>

void print_usage(char const* exe_name);

int main(int argc, char** argv) {
    auto args = parse_args(argc, argv);

    #ifdef DEBUG
    debug_print(std::cout, args);
    #endif

    if (!args.error.empty()) {
        std::cout << "ERROR: " << args.error << '\n';
        print_usage(argv[0]);
        std::exit(EXIT_FAILURE);
    }

    if (args.hide) {
        auto cover_file = Image::load(args.cover_file);

        #ifdef DEBUG
        debug_print(std::cout, cover_file);
        #endif

        if (!cover_file.error.empty()) {
            std::cout << "ERROR: " << cover_file.error << '\n';
            std::exit(EXIT_FAILURE);
        }

        unsigned char const* msg = (unsigned char const*)"hello";
        std::vector<unsigned char> message(msg, msg + 6);
        hide(cover_file, message);

        if (args.output_file.empty()) {
            args.output_file = "data/steg-output.png";
        }

        cover_file.save(args.output_file);
    } else if (args.extract) {
        auto steg_file = Image::load(args.stego_file);

        #ifdef DEBUG
        debug_print(std::cout, steg_file);
        #endif

        auto extracted_message = extract(steg_file);

        std::cout << "[ ";
        for (auto e : extracted_message) {
            std::cout << std::format("{:02x} ", e);
        }
        std::cout << "]\n";
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
