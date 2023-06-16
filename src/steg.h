#ifndef LSB_HIDING_202306161226
#define LSB_HIDING_202306161226

#include "utility.h"
#include "image.h"

struct LSBHider {
    Image* p_img;
    size_t write_index;
    size_t read_index;

    void write_bytes(u8 const* data, size_t count);
    void read_bytes(u8* data_out, size_t count);
};

struct LSBExtractor {
    Image const* p_img;
    size_t read_index;

    void read_bytes(u8* data_out, size_t count);
};

#endif // LSB_HIDING_202306161226
