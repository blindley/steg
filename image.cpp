#include "image.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "utility.h"

#include <format>
#include <cstdint>
#include <iostream>
#include <cassert>

void debug_print(std::ostream& ostr, Image const& img) {
    ostr << "Image { ";
    if (img.error.empty()) {
        ostr << "width=" << img.width << ' ';
        ostr << "height=" << img.height << ' ';
    } else {
        ostr << "error=\"" << img.error << "\" ";
    }
    ostr << "}\n";
}

void Image::save(std::string const& filename) {
    if (stbi_write_png(filename.c_str(), this->width, this->height, 4, this->pixel_data.data(), 0) == 0) {
        std::cout << "failure writing " << filename << '\n';
    } else {
        std::cout << "success writing " << filename << '\n';
    }
}

Image Image::load(std::string const& filename) {
    Image img = {};

    int x, y;
    auto data = stbi_load(filename.c_str(), &x, &y, nullptr, 4);
    if (data == nullptr) {
        img.error = std::format("Unable to load \"{}\".", filename);
    } else {
        img.width = x;
        img.height = y;
        size_t size = x * y * 4;
        img.pixel_data.assign(data, data + size);
        stbi_image_free(data);
    }

    return img;
}

static inline u8 set_lsb(u8 byte_in, bool bit_value) {
    u8 bit_mask = bit_value ? 1 : 0;
    return (byte_in & 0xfe) | bit_mask;
}

struct LSBHider {
    Image* p_img;
    size_t write_index;
    size_t read_index;

    void write_bytes(u8 const* data, size_t count);
};

void LSBHider::write_bytes(u8 const* data, size_t count) {
    for (size_t data_index = 0; data_index < count; data_index++) {
        for (size_t bit_index = 0; bit_index < 8; bit_index++) {
            u8 bit_value = (data[data_index] >> (7 - bit_index)) & 1;

            size_t pixel_index = this->write_index / 3;
            size_t channel = this->write_index % 3;

            size_t offset_in_image = 4 * pixel_index + channel;
            this->p_img->pixel_data[offset_in_image]
                = set_lsb(this->p_img->pixel_data[offset_in_image], bit_value);

            this->write_index++;
        }
    }
}

void hide(Image& img, std::vector<u8> const& message) {
    size_t message_bit_count = message.size() * 8;
    size_t total_bit_count = 32 + message_bit_count;

    size_t max_bit_count = img.width * img.height * 3;
    if (total_bit_count > max_bit_count) {
        img = {};
        img.error = "Error: message is too large for cover file";
        return;
    }

    LSBHider hider = {};
    hider.p_img = &img;
    u32 size = message.size();
    u8 size_vec[4];
    for (size_t i = 0; i < 4; i++) {
        size_vec[i] = (size >> (8 * (3 - i))) & 0xff;
    }

    auto old_pixels = img.pixel_data;
    hider.write_bytes(size_vec, 4);
    hider.write_bytes(message.data(), message.size());
    std::cout << hider.write_index << " bits written\n";
}

std::vector<u8> extract(Image const& img) {
    std::vector<u8> message;
    return message;
}
