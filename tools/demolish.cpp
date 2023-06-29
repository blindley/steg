#include <string>
#include <format>
#include <iostream>

#include "../src/utility.h"
#include "../src/image.h"
#include "../test/bcps_test.h"

void demolish_plane_chunk(DataChunk& chunk);
void demolish_cat(std::string outfile, float complexity_threshold);
size_t count_transparent_pixels(Image const& img);

int main() {
    auto img = Image::load("data/orange-cat-plus-purple.png");
    auto trans_count = count_transparent_pixels(img);
    std::cout << "data/orange-cat-plus-purple.png has " << trans_count << " transparent pixels\n";

    for (int i = 0; i < 8; i++) {
        float complexity_threshold = (float)(i + 1)/20.0f;
        auto outfile = std::format("data/demolished/demolished-{:.02f}.png", complexity_threshold);
        demolish_cat(outfile, complexity_threshold);
    }
}

void demolish_plane_chunk(DataChunk& chunk) {
    for (size_t i = 0; i < 4; i++) {
        chunk.bytes[i * 2] = 0x55;
        chunk.bytes[i * 2 + 1] = 0xAA;
    }
}

void demolish_cat(std::string outfile, float complexity_threshold) {
    auto img = Image::load("data/orange-cat-plus-purple.png");

    auto planed_data = chunk_and_planify(img);

    size_t bytes_written = 0;
    for (auto& chunk : planed_data) {
        auto complexity = measure_plane_chunk_complexity(chunk);
        if (complexity >= complexity_threshold) {
            demolish_plane_chunk(chunk);
            bytes_written += 8;
        }
    }

    de_chunk_and_planify(img, planed_data);
    img.save(outfile);
    auto trans_count = count_transparent_pixels(img);
    std::cout << outfile << " has " << trans_count << " transparent pixels\n";

    std::cout << complexity_threshold << "  " << bytes_written << " bytes hidden\n";
}

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
