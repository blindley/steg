#include <string>
#include <format>
#include <iostream>

#include "../src/utility.h"
#include "../src/image.h"

std::vector<u8> chunk_and_planify(Image const& img);
void de_chunk_and_planify(Image& img, std::vector<u8> const& planed_data);
float measure_plane_chunk_complexity(u8 const* planed_data_chunk);

void demolish_plane_chunk(u8* planed_chunk_data);
void demolish_cat(std::string outfile, float complexity_thresshold);
size_t count_transparent_pixels(Image const& img) {
    size_t count = 0;
    for (size_t i = 0; i < img.pixel_data.size(); i += 4) {
        u8 alpha = img.pixel_data[i + 3];
        if (alpha != 255) {
            count++;
        }
    }
    return count;
}

int main() {
    auto img = Image::load("data/orange-cat-plus-purple.png");
    auto trans_count = count_transparent_pixels(img);
    std::cout << "data/orange-cat-plus-purple.png has " << trans_count << " transparent pixels\n";

    for (int i = 0; i < 8; i++) {
        float complexity_thresshold = (float)(i + 1)/20.0f;
        auto outfile = std::format("data/demolished-{:.02f}.png", complexity_thresshold);
        demolish_cat(outfile, complexity_thresshold);
    }
}

void demolish_plane_chunk(u8* planed_chunk_data) {
    for (size_t i = 0; i < 4; i++) {
        planed_chunk_data[i * 2] = 0x55;
        planed_chunk_data[i * 2 + 1] = 0xAA;
    }
}

void demolish_cat(std::string outfile, float complexity_thresshold) {
    auto img = Image::load("data/orange-cat-plus-purple.png");

    auto planed_data = chunk_and_planify(img);

    size_t bytes_written = 0;
    for (size_t i = 0; i < planed_data.size(); i += 8) {
        auto data_ptr = planed_data.data() + i;
        auto complexity = measure_plane_chunk_complexity(data_ptr);
        if (complexity > complexity_thresshold) {
            demolish_plane_chunk(data_ptr);
            bytes_written += 8;
        }
    }

    de_chunk_and_planify(img, planed_data);
    img.save(outfile);
    auto trans_count = count_transparent_pixels(img);
    std::cout << outfile << " has " << trans_count << " transparent pixels\n";

    std::cout << complexity_thresshold << "  " << bytes_written << " bytes hidden\n";
}
