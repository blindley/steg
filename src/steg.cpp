
#include "steg.h"
#include "bpcs.h"

void hide(Image& img, std::vector<u8> const& message) {
    bpcs_hide_message(0.3, img, message);
}

std::vector<u8> extract(Image& img) {
    return bpcs_unhide_message(0.3, img);
}
