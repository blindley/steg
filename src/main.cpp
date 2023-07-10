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

void print_usage(char const* exe_name);
void print_help(char const* exe_name);

void save_file(std::string const& filename, u8 const* data, size_t len);
void save_file(std::string const& filename, std::vector<u8> const& data);
std::vector<u8> load_file(std::string const& filename);
std::vector<u8> random_bytes(size_t size);
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

    #ifdef DEBUG
    debug_print(std::cout, args);
    #endif

    if (args.help) {
        print_help(argv[0]);
    } else if (args.hide) {
        auto cover_file = Image::load(args.cover_file);

        #ifdef DEBUG
        debug_print(std::cout, cover_file);
        #endif

        std::vector<u8> message;
        if (args.message_file == "--random") {
            auto cover_copy = cover_file;
            auto measure = measure_capacity(0.3, cover_copy);
            message = random_bytes(measure.total_message_capacity);
        } else {
            message = load_file(args.message_file);
        }

        // TODO: figure this out from command line arguments
        u8 rmax = 8, gmax = 8, bmax = 8, amax = 8;

        float threshold = bpcs_hide_message(cover_file, message, rmax, gmax, bmax, amax);

        if (args.output_file.empty()) {
            args.output_file = "data/steg-output.png";
        }

        cover_file.save(args.output_file);
        std::cout << "complexity threshold : " << threshold << '\n';
    } else if (args.extract) {
        auto steg_file = Image::load(args.stego_file);

        #ifdef DEBUG
        debug_print(std::cout, steg_file);
        #endif

        auto extracted_message = bpcs_unhide_message(steg_file);

        if (args.output_file.empty()) {
            args.output_file = "data/message.dat";
        }

        save_file(args.output_file, extracted_message.data(), extracted_message.size());

        std::cout << "extracted " << extracted_message.size() << " bytes to "
            << args.output_file << '\n';
    } else if (args.measure) {
        auto cover_file = Image::load(args.cover_file);
        auto measurements = measure_capacity(args.threshold, cover_file);

        std::cout << "total_capacity: " << measurements.total_message_capacity << '\n';
        std::cout << "complex chunks per bitplane:\n";
        std::cout << "       red     green      blue     alpha\n";
        for (size_t i = 0; i < 8; i++) {
            auto r = measurements.available_chunks_per_bitplane[i];
            auto g = measurements.available_chunks_per_bitplane[8 + i];
            auto b = measurements.available_chunks_per_bitplane[16 + i];
            auto a = measurements.available_chunks_per_bitplane[24 + i];
            for (size_t channel_index = 0; channel_index < 4; channel_index++) {
                auto component_value = measurements.available_chunks_per_bitplane[channel_index * 8 + i];
                std::cout << std::setw(10) << component_value;
            }
            std::cout << '\n';
        }
    } else {
        throw "you shouldn't be here!";
    }
}

std::string get_exe_short_name(char const* argv0) {
    char const* after_last_slash = argv0;
    for (size_t i = 0; argv0[i] != 0; i++) {
        if (argv0[i] == '/' || argv0[i] == '\\') {
            after_last_slash = argv0 + i + 1;
        }
    }
    return after_last_slash;
}

void print_usage(char const* exe_name) {
    auto exe_short_name = get_exe_short_name(exe_name);

    std::cout << "Usage:\n";
    std::cout << "    " << exe_short_name << " --hide -m <message file | --random> -c <coverfile> [-o <stego file>]\n";
    std::cout << "    " << exe_short_name << " --extract -s <stego file> [-o <message file>]\n";
    std::cout << "    " << exe_short_name << " --measure -c <cover file> -t <threshold>\n";
    std::cout << "    " << exe_short_name << " --help\n";
}

void print_help(char const* argv0) {
    print_usage(argv0);

    std::cout << "\n";
    std::cout << "Modes:\n";
    std::cout << "  --hide              Hide message in cover image\n";
    std::cout << "  --extract           Extract hidden message\n";
    std::cout << "  --measure           Measure hiding capacity of an image\n";
    std::cout << "  --help              Display this help message\n";

    std::cout << "\nHide Mode Options:\n";
    std::cout << "  -c <coverfile>      Cover image to hide message in\n";
    std::cout << "  -m <message file>   Message file to hide\n";
    std::cout << "  -m --random         Fill cover file with random data\n";
    std::cout << "  -o <stego file>     Name of output stego image file\n";

    std::cout << "\nExtract Mode Options:\n";
    std::cout << "  -s <stego file>     Stego file to extract hidden message from\n";
    std::cout << "  -o <message file>   Name of output message file\n";
    
    std::cout << "\nMeasure Mode Options:\n";
    std::cout << "  -c <cover file>     Cover image to measure for capacity\n";
    std::cout << "  -t <threshold>      Complexity threshold to measure for [0,0.5]\n";
}

void save_file(std::string const& filename, u8 const* data, size_t len) {
    std::ofstream ofstr(filename, std::ios::binary);
    if (!ofstr) {
        throw std::format("unable to open {} for writing", filename);
    }

    ofstr.write((char const*)data, len);

    if (!ofstr) {
        throw std::format("error writing to {}", filename);
    }
}

void save_file(std::string const& filename, std::vector<u8> const& data) {
    save_file(filename, data.data(), data.size());
}

std::vector<u8> load_file(std::string const& filename) {
    std::ifstream ifstr(filename, std::ios::binary);

    if (!ifstr) {
        throw std::format("unable to open {}", filename);
    }

    ifstr.seekg(0, std::ios::end);
    auto size = ifstr.tellg();
    ifstr.seekg(0, std::ios::beg);

    if (!ifstr) {
        throw std::format("error reading {}", filename);
    }

    std::vector<u8> data;
    data.resize(size);

    ifstr.read((char*)data.data(), size);

    if (!ifstr) {
        throw std::format("error reading {}", filename);
    }

    return data;
}

std::vector<u8> random_bytes(size_t size) {
    std::mt19937_64 gen;
    std::vector<u8> v(size);
    for (auto& e : v) {
        e = gen();
    }
    return v;
}
