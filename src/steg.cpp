
#include "steg.h"

struct LSBHider {
    Image* p_img;
    size_t write_index;

    void write_bytes(u8 const* data, size_t count);
    void read_bytes(u8* data_out, size_t count);
};

struct LSBExtractor {
    Image const* p_img;
    size_t read_index;

    void read_bytes(u8* data_out, size_t count);
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

void LSBExtractor::read_bytes(u8* data_out, size_t count) {
    for (size_t data_index = 0; data_index < count; data_index++) {
        u8 byte_out = 0;
        for (size_t bit_index = 0; bit_index < 8; bit_index++) {
            size_t pixel_index = this->read_index / 3;
            size_t channel = this->read_index % 3;

            size_t offset_in_image = 4 * pixel_index + channel;

            byte_out <<= 1;
            byte_out |= (this->p_img->pixel_data[offset_in_image] & 1);

            this->read_index++;
        }
        data_out[data_index] = byte_out;
    }
}

void hide(Image& img, std::vector<u8> const& message) {
    size_t max_capacity = img.width * img.height * 3 / 8 - sizeof(u32);
    if (message.size() > max_capacity) {
        throw "message is too large for cover file";
    }

    LSBHider hider = {};
    hider.p_img = &img;
    u32 size = message.size();

    u8 size_vec[4];
    u32_to_bytes_be(size, size_vec);

    auto old_pixels = img.pixel_data;
    hider.write_bytes(size_vec, 4);
    hider.write_bytes(message.data(), message.size());
}

std::vector<u8> extract(Image const& img) {
    std::vector<u8> message;

    LSBExtractor extractor = {};
    extractor.p_img = &img;

    u8 buffer[4];
    extractor.read_bytes(buffer, 4);
    u32 size = u32_from_bytes_be(buffer);

    message.resize(size);
    extractor.read_bytes(message.data(), size);

    return message;
}
