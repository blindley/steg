#include <iostream>
#include <random>
#include <fstream>
#include <string>
#include <cstdlib>

void main_impl(int argc, char** argv);

int main(int argc, char** argv) {
    try {
        main_impl(argc, argv);
    } catch (char const* e) {
        std::cerr << "Error: " << e << '\n';
    } catch (std::string const& e) {
        std::cerr << "Error: " << e << '\n';
    }
}

void main_impl(int argc, char** argv) {
    if (argc != 3)
        throw "wrong number of arguments";

    auto num_bytes = std::stoull(argv[1]);

    std::random_device rd;
    std::mt19937_64 gen(rd() + std::time(nullptr));

    std::vector<char> data;
    data.reserve(num_bytes);
    for (size_t i = 0; i < num_bytes; i++) {
        data.push_back(gen());
    }

    std::ofstream fout(argv[2], std::ios::binary);
    if (!fout) {
        std::string message = "unable to open ";
        message += argv[2];
        throw message;
    }

    fout.write(data.data(), data.size());
}
