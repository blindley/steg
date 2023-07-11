#include "image.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "utility.h"

#include <format>
#include <cstdint>
#include <iostream>
#include <cassert>

void Image::save(std::string const& filename) {
    if (stbi_write_png(filename.c_str(), this->width, this->height, 4, this->pixel_data.data(), 0) == 0) {
        auto err = std::format("failure writing {}", filename);
        throw std::runtime_error(err);
    } else {
        std::cout << "success writing " << filename << '\n';
    }
}

Image Image::load(std::string const& filename) {
    Image img = {};

    int x, y;
    auto data = stbi_load(filename.c_str(), &x, &y, nullptr, 4);
    if (data == nullptr) {
        auto err = std::format("unable to load \"{}\"", filename);
        throw std::runtime_error(err);
    } else {
        img.width = x;
        img.height = y;
        size_t size = x * y * 4;
        img.pixel_data.assign(data, data + size);
        stbi_image_free(data);
    }

    return img;
}
