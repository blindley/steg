
#include "steg.h"
#include "lsb.h"

void hide(Image& img, std::vector<u8> const& message) {
    hide_lsb(img, message);
}

std::vector<u8> extract(Image const& img) {
    return extract_lsb(img);
}
