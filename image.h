#include <ostream>
#include <vector>
#include <string>

struct Image {
    int width;
    int height;
    std::vector<unsigned char> pixel_data;
    std::string error;
};

void debug_print(std::ostream& ostr, Image const& img);
Image load_image(std::string const& filename);
