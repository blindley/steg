
#include "steg.h"
#include "lsb.h"
#include "bcps.h"

void hide(Image& img, std::vector<u8> const& message) {
    bcps_hide_message(0.3, img, message);
}

std::vector<u8> extract(Image const& img) {
    return bcps_unhide_message(0.3, img);
}
