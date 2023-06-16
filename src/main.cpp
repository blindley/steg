#include "args.h"
#include "image.h"
#include "lsb_hiding.h"
#include "utility.h"

#include <iostream>
#include <vector>
#include <string>
#include <format>

void print_usage(char const* exe_name);
void hide(Image& img, std::vector<u8> const& message);
std::vector<u8> extract(Image const& img);

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

void hide(Image& img, std::vector<u8> const& message) {
    size_t message_bit_count = message.size() * 8;
    size_t total_bit_count = 32 + message_bit_count;

    size_t max_bit_count = img.width * img.height * 3;
    if (total_bit_count > max_bit_count) {
        img = {};
        img.error = "Error: message is too large for cover file";
        return;
    }

    LSBHider hider = {};
    hider.p_img = &img;
    u32 size = message.size();
    u8 size_vec[4];
    for (size_t i = 0; i < 4; i++) {
        size_vec[i] = (size >> (8 * (3 - i))) & 0xff;
    }

    auto old_pixels = img.pixel_data;
    hider.write_bytes(size_vec, 4);
    hider.write_bytes(message.data(), message.size());
    std::cout << hider.write_index << " bits written\n";
}

std::vector<u8> extract(Image const& img) {
    std::vector<u8> message;

    LSBExtractor extractor = {};
    extractor.p_img = &img;

    u8 buffer[4];
    extractor.read_bytes(buffer, 4);
    u32 size = u32_from_bytes_be(buffer);

    message.resize(size);
    extractor.read_bytes(message.data(), size);

    return message;
}
