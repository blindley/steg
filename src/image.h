#ifndef IMAGE_202306151742
#define IMAGE_202306151742

#include "utility.h"

#include <ostream>
#include <vector>
#include <string>

struct Image {
    int width;
    int height;
    std::vector<u8> pixel_data;
    std::string error;

    void save(std::string const& filename);
    static Image load(std::string const& filename);
};

void debug_print(std::ostream& ostr, Image const& img);
void hide(Image& img, std::vector<u8> const& message);
std::vector<u8> extract(Image const& img);

#endif // IMAGE_202306151742
