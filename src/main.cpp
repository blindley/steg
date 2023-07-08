#include "args.h"
#include "image.h"
#include "utility.h"
#include "bpcs.h"

#include <iostream>
#include <vector>
#include <string>
#include <format>
#include <fstream>
#include <iomanip>
#include <random>

float const COMPLEXITY_THRESHOLD = 0.3;

void print_usage(char const* exe_name);
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

    if (args.hide) {
        auto cover_file = Image::load(args.cover_file);

        #ifdef DEBUG
        debug_print(std::cout, cover_file);
        #endif

        std::vector<u8> message;
        if (args.message_file == "--random") {
            auto cover_copy = cover_file;
            auto measure = measure_capacity(COMPLEXITY_THRESHOLD, cover_copy);
            message = random_bytes(measure.total_message_capacity);
        } else {
            message = load_file(args.message_file);
        }

        bpcs_hide_message(COMPLEXITY_THRESHOLD, cover_file, message);

        if (args.output_file.empty()) {
            args.output_file = "data/steg-output.png";
        }

        cover_file.save(args.output_file);
    } else if (args.extract) {
        auto steg_file = Image::load(args.stego_file);

        #ifdef DEBUG
        debug_print(std::cout, steg_file);
        #endif

        auto extracted_message = bpcs_unhide_message(COMPLEXITY_THRESHOLD, steg_file);

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

void print_usage(char const* exe_name) {
    char const* after_last_slash = exe_name;
    for (size_t i = 0; exe_name[i] != 0; i++) {
        if (exe_name[i] == '/' || exe_name[i] == '\\') {
            after_last_slash = exe_name + i + 1;
        }
    }

    std::cout << "Usage:\n";
    std::cout << "    " << after_last_slash << " --hide -m <message file | --random> -c <coverfile> [-o <stego file>]\n";
    std::cout << "    " << after_last_slash << " --extract -s <stego file> [-o <message file>]\n";
    std::cout << "    " << after_last_slash << " --measure -c <cover file> -t <threshold>\n";
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
