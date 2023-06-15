#include "args.h"
#include "image.h"

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
        std::vector<unsigned char> message(msg, msg + 5);
        hide(cover_file, message);

        cover_file.save("data/steg_output.png");

        auto steg_file = Image::load("data/steg_output.png");
        unsigned int size = 0;
        for (size_t i = 0; i < 32; i++) {
            size_t pixel_data_index = i * 4 / 3;
            size <<= 1;
            size |= (steg_file.pixel_data[pixel_data_index] & 1);
        }

        char buffer[100] = {};
        for (size_t i = 0; i < size; i++) {
            for (size_t j = 0; j < 8; j++) {
                size_t pixel_data_index = (32 + i * 8 + j) * 4 / 3;
                buffer[i] <<= 1;
                buffer[i] |= (steg_file.pixel_data[pixel_data_index] & 1);
            }
        }

        std::cout << buffer << '\n';
    } else if (args.extract) {
        std::cout << "extract not yet implemented\n";
        std::exit(EXIT_FAILURE);
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
