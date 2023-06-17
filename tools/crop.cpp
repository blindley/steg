#include "../src/utility.h"
#include "../src/image.h"

#include <iostream>
#include <vector>
#include <string>
#include <format>

void main_impl(int argc, char** argv);
void print_usage(std::string const& exe_name);
std::string get_extension_lowercase(std::string const& filename);

int main(int argc, char** argv) {
    std::string error;
    try {
        main_impl(argc, argv);
    } catch(char const* e) {
        error = e;
    } catch(std::string e) {
        error = e;
    } catch(std::exception const& e) {
        error = e.what();
    } catch(...) {
        error = "unknown error";
    }

    if (!error.empty()) {
        std::cerr << "ERROR: " << error << '\n';
        print_usage(argv[0]);
        std::exit(EXIT_FAILURE);
    }
}

void main_impl(int argc, char** argv) {
    std::vector<std::string> arg_vec(argv, argv + argc);

    if (arg_vec.size() != 6) {
        throw "wrong number of arguments";
    }

    std::string image_filename = arg_vec[1];

    int crop_x = std::stoi(arg_vec[2]);
    int crop_y = std::stoi(arg_vec[3]);
    int crop_w = std::stoi(arg_vec[4]);
    int crop_h = std::stoi(arg_vec[5]);

    auto img = Image::load(image_filename);

    if (crop_x < 0 || crop_y < 0 || crop_w < 0 || crop_h < 0) {
        throw "invalid cropping dimensions";
    }

    if (crop_x + crop_w > img.width || crop_y + crop_h > img.height) {
        throw "invalid cropping dimensions";
    }

    Image new_image = {};
    new_image.width = crop_w;
    new_image.height = crop_h;

    size_t pixel_count = crop_w * crop_h;
    size_t pixel_data_size = pixel_count * 4;
    new_image.pixel_data.resize(pixel_data_size);

    for (int y = 0; y < crop_h; y++) {
        size_t src_pixel_offset = (y + crop_y) * img.width + crop_x;
        u8 const* src_pixel_ptr = img.pixel_data.data() + src_pixel_offset * 4;

        size_t dest_pixel_offset = y * crop_w;
        u8* dest_pixel_ptr = new_image.pixel_data.data() + dest_pixel_offset * 4;

        size_t crop_byte_width = crop_w * 4;
        std::memcpy(dest_pixel_ptr, src_pixel_ptr, crop_byte_width);
    }

    new_image.save("data/crop.png");
}

void print_usage(std::string const& exe_name) {
    std::cout << "Usage:\n";
    std::cout << std::format("    {} <image file> <x> <y> <w> <h>\n", exe_name);
}
