#include <cstdint>
#include <iostream>
#include <cassert>
#include <sstream>

#include <stb_image.h>
#include <stb_image_write.h>

#include "image.h"
// #include "stb/stb_image.h"
// #include "stb/stb_image_write.h"
#include "utility.h"

void Image::save(std::string const& filename) {
    auto ext = get_file_extension(filename);

    int success;
    if (ext == "bmp") 
        success = stbi_write_bmp(filename.c_str(), this->width, this->height, 4, this->pixel_data.data());
    else if (ext == "png")
        success = stbi_write_png(filename.c_str(), this->width, this->height, 4, this->pixel_data.data(), 0);
    else if (ext == "tga")
        success = stbi_write_tga(filename.c_str(), this->width, this->height, 4, this->pixel_data.data());
    else {
        std::ostringstream oss;
        oss << "unsupported file extension ." << ext;
        auto err = oss.str();
        throw std::runtime_error(err);
    }
    
    if (success) {
        std::cout << "success writing " << filename << '\n';
    } else {
        auto reason = stbi_failure_reason();
        std::ostringstream oss;
        oss << "failure writing \"" << filename << "\"; reason: " << reason;
        auto err = oss.str();
        throw std::runtime_error(err);
    }
}

Image Image::load(std::string const& filename) {
    Image img = {};

    int x, y;
    auto data = stbi_load(filename.c_str(), &x, &y, nullptr, 4);
    if (data == nullptr) {
        auto reason = stbi_failure_reason();
        std::ostringstream oss;
        oss << "unable to load \"" << filename << "\"; reason: " << reason;
        auto err = oss.str();
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
