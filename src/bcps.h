#ifndef BCPS_202306171341
#define BCPS_202306171341

#include "image.h"
#include "utility.h"
#include <vector>

void bcps_hide_message(float threshold, Image& img, std::vector<u8> const& message);
std::vector<u8> bcps_unhide_message(float threshold, Image const& img);

#endif // BCPS_202306171341
