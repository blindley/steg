#include <ostream>
#include <vector>
#include <string>

struct Image {
    int width;
    int height;
    std::vector<unsigned char> pixel_data;
    std::string error;

    void save(std::string const& filename);
    static Image load(std::string const& filename);
};

void debug_print(std::ostream& ostr, Image const& img);
void hide(Image& img, std::vector<unsigned char> const& message);
std::vector<unsigned char> extract(Image const& img);
