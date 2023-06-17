#ifndef LSB_202306171330
#define LSB_202306171330

#include "utility.h"
#include "image.h"

#include <vector>

void hide_lsb(Image& img, std::vector<u8> const& message);
std::vector<u8> extract_lsb(Image const& img);

#endif // LSB_202306171330
