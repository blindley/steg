#include "../src/utility.h"
#include "../src/image.h"

std::vector<u8> chunk_and_planify(Image const& img);
void de_chunk_and_planify(Image& img, std::vector<u8> const& planed_data);

int main() {
    auto img = Image::load("data/orange-cat-plus-purple.png");

    auto planed_data = chunk_and_planify(img);

    de_chunk_and_planify(img, planed_data);
    img.save("data/exact-copy.png");

    auto no_red = planed_data;
    size_t offset = 0;
    for (size_t i = 0; i < planed_data.size() / 4; i++) {
        no_red[offset + i] = 0;
    }
    de_chunk_and_planify(img, no_red);
    img.save("data/no-red.png");

    auto no_green = planed_data;
    offset = planed_data.size() / 4;
    for (size_t i = 0; i < planed_data.size() / 4; i++) {
        no_green[planed_data.size() / 4 + i] = 0;
    }
    de_chunk_and_planify(img, no_green);
    img.save("data/no-green.png");

    auto no_blue = planed_data;
    offset = 2 * planed_data.size() / 4;
    for (size_t i = 0; i < planed_data.size() / 4; i++) {
        no_blue[offset + i] = 0;
    }
    de_chunk_and_planify(img, no_blue);
    img.save("data/no-blue.png");

    auto so_red = planed_data;
    offset = 0;
    for (size_t i = 0; i < planed_data.size() / 4; i++) {
        size_t bit_index = i / (planed_data.size() / 32);
        if (bit_index == 0)
            so_red[offset + i] = 255;
        else
            so_red[offset + i] = 0;
    }
    de_chunk_and_planify(img, so_red);
    img.save("data/so-red.png");
}
