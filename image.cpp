#include "image.h"

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

Image load_image(char const* filename) {
    Image img = {};
    img.error = "load_image not yet implemented";
    return img;
}
