#ifndef LSB_HIDING_202306161226
#define LSB_HIDING_202306161226

#include "utility.h"
#include "image.h"

#include <vector>

void hide(Image& img, std::vector<u8> const& message);
std::vector<u8> extract(Image const& img);

#endif // LSB_HIDING_202306161226
