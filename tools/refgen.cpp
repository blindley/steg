
#include "../src/image.h"
#include "../src/utility.h"

#include <iostream>

bool is_prime(int n);
int next_prime_after(int n);
void copy3(u8* dest, u8 const* src);
void copy4(u8* dest, u8 const* src);

int main() {
    Image img = {};
    img.width = next_prime_after(512);
    img.height = next_prime_after(256);

    size_t pixel_count = img.width * img.height;
    size_t pixel_data_size = pixel_count * 4;
    img.pixel_data.resize(pixel_data_size);

    u8 const dark_color[] = {15, 16, 17};
    u8 const light_color[] = {47, 48, 49};
    u8 const* alt_colors[] = {dark_color, light_color};

    for (size_t pixel_index = 0; pixel_index < pixel_count; pixel_index++) {
        size_t y = pixel_index / img.width;
        size_t x = pixel_index % img.width;

        u8* pixel_ptr = img.pixel_data.data() + pixel_index * 4;

        size_t chunk_x = x / 8;
        size_t chunk_y = y / 8;
        size_t color_index = (chunk_x + chunk_y) % 2;
        u8 const* color = alt_colors[color_index];

        copy3(pixel_ptr, color);

        size_t alpha_index = (x + y) % 2;
        pixel_ptr[3] = alpha_index ? 255 : 254;
    }

    std::string filename = "data/checkerboard.png";
    std::cout << "saving reference image to " << filename << '\n';
    img.save(filename);
}

bool is_prime(int n) {
    for (int i = 2; i < n; i++) {
        if (n % i == 0)
            return false;
    }
    return true;
}

int next_prime_after(int n) {
    while (true) {
        n++;
        if (is_prime(n))
            return n;
    }
}

void copy3(u8* dest, u8 const* src) {
    for (size_t i = 0; i < 3; i++)
        dest[i] = src[i];
}

void copy4(u8* dest, u8 const* src) {
    for (size_t i = 0; i < 4; i++)
        dest[i] = src[i];
}
