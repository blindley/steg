#include "args.h"
#include "image.h"
#include "steg.h"
#include "utility.h"

#include <iostream>
#include <vector>
#include <string>
#include <format>
#include <fstream>

void print_usage(char const* exe_name);
void save_file(std::string const& filename, u8 const* data, size_t len);
void save_file(std::string const& filename, std::vector<u8> const& data);
std::vector<u8> load_file(std::string const& filename);
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

        auto message = load_file(args.message_file);
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

        if (args.output_file.empty()) {
            args.output_file = "data/message.dat";
        }

        save_file(args.output_file, extracted_message.data(), extracted_message.size());

        std::cout << "extracted " << extracted_message.size() << " bytes to "
            << args.output_file << '\n';
    } else {
        throw "you shouldn't be here!";
    }
}

void print_usage(char const* exe_name) {
    std::cout << "Usage:\n";
    std::cout << "    " << exe_name << " --hide -m <message file> -c <coverfile> [-o <stego file>]\n";
    std::cout << "    " << exe_name << " --extract -s <stego file> [-o <message file]\n";
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
