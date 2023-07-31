// Benjamin Lindley, Vanessa Martinez
//
// image.cpp
//
// Uses the stb image libraries to load and save images: https://github.com/nothings/stb
//
// All images are upscaled to 32-bit rgba. Images without an alpha channel are given a fully opaque
// alpha channel. Grayscale images have their pixel values duplicated across red, green and blue.
// Paletted images are depalettized, with each pixel value being replaced by the corresponding value
// from its palette entry.
//
// Most common image formats can be loaded correctly. There are some rare edge cases. They are
// described in https://raw.githubusercontent.com/nothings/stb/master/stb_image.h. For saving, we
// only allow bmp, png and tga. Saving in a lossy format such as jpg might destroy the hidden
// message, and so is not permitted. There are some limits on image dimensions, but these are not
// limits which are likely to be an issue for any real images. These limits are also described in
// stb_image.h. Note that even though the bpcs algorithm works on 8x8 pixel chunks, the image is not
// required to have dimensions which are a multiple of 8. Pixels on the right and bottom edge which
// go beyond a multiple of 8 are simply left untouched.

#include <cstdint>
#include <iostream>
#include <cassert>
#include <sstream>

// The STB libraries produce several warnings. Temporarily disable them.

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4996)
#pragma warning(disable:4244)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "declarations.h"

void Image::save(std::string const& filename) {
    auto ext = get_file_extension(filename);

    int success;
    int w = (int)this->width;
    int h = (int)this->height;
    if (ext == "bmp") 
        success = stbi_write_bmp(filename.c_str(), w, h, 4, this->pixel_data.data());
    else if (ext == "png")
        success = stbi_write_png(filename.c_str(), w, h, 4, this->pixel_data.data(), 0);
    else if (ext == "tga")
        success = stbi_write_tga(filename.c_str(), w, h, 4, this->pixel_data.data());
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
