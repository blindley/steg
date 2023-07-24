#include "args.h"
#include "image.h"
#include "utility.h"
#include "bpcs.h"
#include "message.h"

#include <iostream>
#include <vector>
#include <string>
#include <format>
#include <fstream>
#include <iomanip>
#include <random>

void main_impl(int argc, char** argv);

int main(int argc, char** argv) {
    std::string error;
    try {
        main_impl(argc, argv);
    } catch (std::exception const& e) {
        error = e.what();
    } catch (std::string e) {
        error = e;
    } catch (char const* e) {
        error = e;
    } catch (...) {
        error = "unknown error";
    }

    if (!error.empty()) {
        std::cerr << "ERROR: " << error << '\n';
        print_usage(argv[0]);
        std::exit(EXIT_FAILURE);
    }
}

void main_impl(int argc, char** argv) {
    auto args = parse_args(argc, argv);

    if (args.help) {
        print_help(argv[0]);
    } else if (args.hide) {
        auto cover_file = Image::load(args.cover_file);

        std::vector<u8> message;
        if (args.random_count >= 0) {
            message = random_bytes(args.random_count);
        } else {
            if (args.message_file == "-") {
                std::string line;
                while (std::getline(std::cin, line)) {
                    u8 const* b = (u8*)line.data();
                    u8 const* e = b + line.size();
                    message.insert(message.end(), b, e);
                    message.push_back((u8)'\n');
                }
            } else {
                message = load_file(args.message_file);
            }
        }

        auto stats = bpcs_hide_message(cover_file, message,
            args.rmax, args.gmax, args.bmax, args.amax);

        if (args.output_file.empty()) {
            args.output_file = "data/steg-output.png";
        }

        cover_file.save(args.output_file);

        std::cout << "bytes hidden: "
            << stats.message_bytes_hidden << '/' << stats.message_size << '\n';
        std::cout << "chunks used per bitplane:\n";
        std::cout << "       red     green      blue     alpha\n";
        for (size_t i = 0; i < 8; i++) {
            auto a = stats.chunks_used_per_bitplane[24 + i];
            for (size_t channel_index = 0; channel_index < 4; channel_index++) {
                auto component_value = stats.chunks_used_per_bitplane[channel_index * 8 + i];
                std::cout << std::setw(10) << component_value;
            }
            std::cout << '\n';
        }
    } else if (args.extract) {
        auto steg_file = Image::load(args.stego_file);

        #ifdef DEBUG
        debug_print(std::cout, steg_file);
        #endif

        auto extracted_message = bpcs_unhide_message(steg_file);

        if (args.output_file == "-") {
            auto data = (char const*)extracted_message.data();
            std::cout.write(data, extracted_message.size());
        } else {
            if (args.output_file.empty()) {
                args.output_file = "data/message.dat";
            }

            save_file(args.output_file, extracted_message.data(), extracted_message.size());

            std::cout << "extracted " << extracted_message.size() << " bytes to "
                << args.output_file << '\n';
        }
    } else if (args.measure) {
        auto cover_file = Image::load(args.cover_file);
        auto stats = measure_capacity(args.threshold, cover_file,
            args.rmax, args.gmax, args.bmax, args.amax);

        std::cout << "total_capacity: " << stats.message_bytes_hidden << '\n';
        std::cout << "complex chunks per bitplane:\n";
        std::cout << "       red     green      blue     alpha\n";
        for (size_t i = 0; i < 8; i++) {
            for (size_t channel_index = 0; channel_index < 4; channel_index++) {
                auto component_value = stats.chunks_used_per_bitplane[channel_index * 8 + i];
                std::cout << std::setw(10) << component_value;
            }
            std::cout << '\n';
        }
    } else {
        auto err = "you shouldn't be here!";
        throw std::logic_error(err);
    }
}
