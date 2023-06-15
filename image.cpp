#include "image.h"
#include "stb/stb_image.h"

#include <format>

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

Image load_image(std::string const& filename) {
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
