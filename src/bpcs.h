#ifndef BCPS_202306171341
#define BCPS_202306171341

#include "image.h"
#include "utility.h"
#include "datachunk.h"

#include <vector>

struct Measurements {
    size_t total_message_capacity;
    size_t available_chunks_per_bitplane[32];
};

float bpcs_hide_message(Image& img, std::vector<u8> const& message);
std::vector<u8> bpcs_unhide_message(Image& img);
Measurements measure_capacity(float threshold, Image& img);

#endif // BCPS_202306171341
