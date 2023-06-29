#include <iostream>

#include "../src/utility.h"
#include "../src/image.h"

#include "../test/bcps_test.h"

int main() {
    std::cerr << "the output of this program is meaningless after adding bitplane priority\n";
    std::exit(1);

    auto img = Image::load("data/orange-cat-plus-purple.png");

    auto planed_data = chunk_and_planify(img);

    std::cout << "planed_data.chunks.size() == " << planed_data.chunks.size() << '\n';

    de_chunk_and_planify(img, planed_data);
    img.save("data/red/exact-copy.png");

    auto no_red = planed_data;
    size_t offset = 0;
    for (size_t i = 0; i < planed_data.chunks.size() / 4; i++) {
        std::memset(no_red.chunks[i].bytes, 0, 8);
    }
    de_chunk_and_planify(img, no_red);
    img.save("data/red/no-red.png");

    auto no_green = planed_data;
    offset = planed_data.chunks.size() / 4;
    for (size_t i = 0; i < planed_data.chunks.size() / 4; i++) {
        no_green.chunks[offset + i] = {};
    }
    de_chunk_and_planify(img, no_green);
    img.save("data/red/no-green.png");

    auto no_blue = planed_data;
    offset = 2 * planed_data.chunks.size() / 4;
    for (size_t i = 0; i < planed_data.chunks.size() / 4; i++) {
        no_blue.chunks[offset + i] = {};
    }
    de_chunk_and_planify(img, no_blue);
    img.save("data/red/no-blue.png");

    auto so_red = planed_data;
    offset = 0;
    for (size_t i = 0; i < planed_data.chunks.size() / 4; i++) {
        size_t bit_index = i / (planed_data.chunks.size() / 32);
        if (bit_index == 0)
            std::memset(so_red.chunks[offset + i].bytes, 0xFF, 8);
        else
            so_red.chunks[offset + i] = {};
    }
    de_chunk_and_planify(img, so_red);
    img.save("data/red/so-red.png");
}

