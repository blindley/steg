
#include "lsb_hiding.h"

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
