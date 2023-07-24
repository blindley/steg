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

struct HideStats {
    float threshold;
    size_t chunks_used;
    size_t chunks_used_per_bitplane[32];
    size_t message_size;
    size_t message_bytes_hidden;
};

HideStats bpcs_hide_message(Image& img, std::vector<u8> const& message,
    u8 rmax, u8 gmax, u8 bmax, u8 amax);
std::vector<u8> bpcs_unhide_message(Image& img);
HideStats measure_capacity(float threshold, Image& img, u8 rmax, u8 gmax, u8 bmax, u8 amax);

#endif // BCPS_202306171341
