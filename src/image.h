#ifndef IMAGE_202306151742
#define IMAGE_202306151742

#include "utility.h"

#include <vector>
#include <string>

struct Image {
    size_t width;
    size_t height;
    std::vector<u8> pixel_data;

    void save(std::string const& filename);
    static Image load(std::string const& filename);
};

#endif // IMAGE_202306151742
