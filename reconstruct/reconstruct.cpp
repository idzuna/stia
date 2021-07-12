
#include "../libpng/png.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <tuple>
#include <regex>
#include <filesystem>

using sample_type = unsigned char;
using width_type = size_t;
using height_type = size_t;
using image_type = std::tuple<std::vector<sample_type>, width_type, height_type>;

void write_png_to_file(sample_type* image, width_type width, height_type height, FILE* fp) {
  std::vector<png_bytep> rows(height);
  for (size_t y = 0; y < height; y++) {
    rows[y] = image + 4 * static_cast<size_t>(width) * y;
  }
  auto png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  auto info = png_create_info_struct(png);
  if (!setjmp(png_jmpbuf(png))) {
    png_init_io(png, fp);
    png_set_IHDR(png, info, static_cast<png_uint_32>(width), static_cast<png_uint_32>(height), 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_rows(png, info, rows.data());
    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, nullptr);
  }
  png_destroy_write_struct(&png, &info);
}

image_type read_png_from_file(const char* filename) {
  FILE* file;
  width_type width = 0;
  height_type height = 0;
  std::vector<sample_type> image;
  if (fopen_s(&file, filename, "rb") || !file) {
    return { image, width, height };
  }
  auto png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  auto info = png_create_info_struct(png);
  if (!setjmp(png_jmpbuf(png))) {
    png_init_io(png, file);
    png_read_png(png, info, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_GRAY_TO_RGB, nullptr);
    if (png_get_color_type(png, info) == PNG_COLOR_TYPE_RGB) {
      width = static_cast<width_type>(png_get_image_width(png, info));
      height = static_cast<height_type>(png_get_image_height(png, info));
      image.resize(width * height * 4);
      const png_bytepp rows = png_get_rows(png, info);
      auto p = image.begin();
      for (height_type y = 0; y < height; y++) {
        auto row = rows[y];
        for (width_type x = 0; x < width; x++) {
          *(p++) = *(row++);
          *(p++) = *(row++);
          *(p++) = *(row++);
          *(p++) = 0xff;
        }
      }
    } else if (png_get_color_type(png, info) == PNG_COLOR_TYPE_RGB_ALPHA) {
      width = static_cast<width_type>(png_get_image_width(png, info));
      height = static_cast<height_type>(png_get_image_height(png, info));
      image.resize(width * height * 4);
      const png_bytepp rows = png_get_rows(png, info);
      auto p = image.begin();
      for (height_type y = 0; y < height; y++) {
        memcpy_s(&*p, width * 4, rows[y], width * 4);
        p += width * 4;
      }
    }
  }
  fclose(file);
  png_destroy_read_struct(&png, &info, nullptr);
  return { image, width, height };
}

image_type reconstruct(const std::string& prefix, const char* filename) {
  std::ifstream input(filename);
  if (!input) {
    return { std::vector<sample_type>(), 0, 0 };
  }
  std::string png_filename;
  std::string origin_filename;
  std::getline(input, png_filename);
  std::getline(input, origin_filename);
  const auto png_path = prefix + png_filename;
  if (input) {
    size_t left, top;
    input >> left;
    input >> top;
    const auto origin_path = prefix + origin_filename;
    auto [ base_image, base_width, base_height ] = reconstruct(prefix, origin_path.c_str());
    if (base_image.empty()) {
      return { std::vector<sample_type>(), 0, 0 };
    }
    const auto [ over_image, over_width, over_height ] = read_png_from_file(png_path.c_str());
    if (over_image.empty()) {
      return { std::vector<sample_type>(), 0, 0 };
    }
    size_t over_cur = 0;
    for (size_t y = 0; y < over_height; ++y) {
      size_t base_cur = (top + y) * base_width * 4 + left * 4;
      for (size_t x = 0; x < over_width; ++x) {
        if (over_image[over_cur + 3] != 0) {
          base_image[base_cur] = over_image[over_cur];
          base_image[base_cur + 1] = over_image[over_cur + 1];
          base_image[base_cur + 2] = over_image[over_cur + 2];
          base_image[base_cur + 3] = over_image[over_cur + 3];
        }
        over_cur += 4;
        base_cur += 4;
      }
    }
    return { base_image, base_width, base_height };
  } else {
    return read_png_from_file(png_path.c_str());
  }
}

void print_usage() {
  std::cout << "usage: reconstruct [-o output_dir] input1.stir input2.stir ..." << std::endl;
}

int main(int argc, char** argv) {
  std::string output_dirname("reconstructed");
  std::vector<std::string> input_files;
  if (argc < 2) {
    print_usage();
    return 0;
  }
  int i = 1;
  while (i < argc) {
    if (strcmp(argv[i], "-o") == 0) {
      ++i;
      if (i >= argc) {
        print_usage();
        return 0;
      }
      output_dirname = argv[i];
      ++i;
    } else {
      input_files.push_back(argv[i]);
      ++i;
    }
  }
  std::filesystem::create_directory(output_dirname);
  for (const auto& filename : input_files) {
    const auto delim = filename.find_last_of("/\\");
    const auto prefix = (delim == std::string::npos) ? std::string() : filename.substr(0, delim + 1);
    const auto basename = (delim == std::string::npos) ? filename : filename.substr(delim + 1);
    const auto ext = basename.find_first_of(".");
    const auto stem = (ext == std::string::npos) ? basename : basename.substr(0, ext);
    auto image = reconstruct(prefix, filename.c_str());
    if (std::get<0>(image).empty()) {
      std::cerr << "failed to reconstruct \"" << filename << "\"" << std::endl;
      return -1;
    }
    auto output_filename = output_dirname + "/" + stem + ".png";
    FILE* fp;
    if (fopen_s(&fp, output_filename.c_str(), "wb") || !fp) {
      std::cerr << "failed to write \"" << output_filename << "\"" << std::endl;
      return -1;
    }
    write_png_to_file(std::get<0>(image).data(), std::get<1>(image), std::get<2>(image), fp);
    fclose(fp);
  }
  return 0;
}
