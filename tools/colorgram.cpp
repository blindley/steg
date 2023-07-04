#include <algorithm>
#include <iostream>
#include <string>
#include <cstdint>
#include <array>

#include "../src/image.h"

using u8 = std::uint8_t;

void main_impl(int argc, char** argv);

int main(int argc, char** argv) {
    try {
        main_impl(argc, argv);
    } catch (char const* e) {
        std::cerr << "ERROR: " << e << '\n';
    } catch (std::string const& e) {
        std::cerr << "ERROR: " << e << '\n';
    }
}

void main_impl(int argc, char** argv) {
    if (argc != 2) {
        throw "wrong number of arguments";
    }

    auto image = Image::load(argv[1]);

    Image histogram_img;
    histogram_img.width = 768;
    histogram_img.height = 512;
    histogram_img.pixel_data.resize(histogram_img.width * histogram_img.height * 4);

    size_t histogram[768] = {};

    for (size_t i = 0; i < image.width * image.height; i++) {
        auto pixel_ptr = image.pixel_data.data() + i * 4;
        for (size_t channel_index = 0; channel_index < 3; channel_index++) {
            auto value = pixel_ptr[channel_index];
            histogram[256 * channel_index + value]++;
        }
    }

    auto highest_frequency_index = std::max_element(histogram, histogram + 768) - histogram;
    auto highest_frequency = histogram[highest_frequency_index];

    std::array<u8, 4> bar_color = {};
    for (size_t i = 0; i < 768; i++) {
        if (i % 256 == 0) {
            switch (i / 256) {
                case 0: bar_color = { 255, 0, 0, 255 }; break;
                case 1: bar_color = { 0, 255, 0, 255 }; break;
                case 2: bar_color = { 0, 0, 255, 255 }; break;
            }
        }

        size_t bar_height = histogram_img.height * histogram[i] / highest_frequency;
        for (size_t j = 0; j < histogram_img.height; j++) {
            auto pixel_ptr = histogram_img.pixel_data.data() + j * histogram_img.width * 4 + i * 4;
            if (histogram_img.height - j > bar_height) {
                std::memset(pixel_ptr, 255, 4);
            } else {
                std::memcpy(pixel_ptr, bar_color.data(), 4);
            }
        }
    }

    histogram_img.save("colorgram.png");
}
