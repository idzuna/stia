
#include "../libpng/png.h"
#include <vector>
#include <iostream>
#include <cstdio>
#include <tuple>
#include <fstream>
#include <cstring>
#include <algorithm>

using sample_type = unsigned char;
using width_type = size_t;
using height_type = size_t;
using image_type = std::tuple<std::vector<sample_type>, width_type, height_type>;

size_t calc_png_size(sample_type* image, width_type width, height_type height) {
  size_t size = 0;
  std::vector<png_bytep> rows(height);
  for (size_t y = 0; y < height; y++) {
    rows[y] = image + 4 * static_cast<size_t>(width) * y;
  }
  const auto png_rw = [](png_structp png, png_bytep byte, size_t size) {
    *static_cast<size_t*>(png_get_io_ptr(png)) += size;
  };
  const auto png_flush = [](png_structp png_ptr) {};
  auto png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  auto info = png_create_info_struct(png);
  if (!setjmp(png_jmpbuf(png))) {
    png_set_write_fn(png, &size, png_rw, png_flush);
    png_set_IHDR(png, info, static_cast<png_uint_32>(width), static_cast<png_uint_32>(height), 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_rows(png, info, rows.data());
    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, nullptr);
  }
  png_destroy_write_struct(&png, &info);
  return size;
}

size_t calc_diff_size(sample_type* from, sample_type* to, width_type width, height_type height) {
  std::vector<sample_type> diff(width * height * 4);
  size_t left = std::numeric_limits<size_t>::max();
  size_t top = std::numeric_limits<size_t>::max();
  size_t right = std::numeric_limits<size_t>::max();
  size_t bottom = std::numeric_limits<size_t>::max();
  uint32_t* f = reinterpret_cast<uint32_t*>(from);
  uint32_t* t = reinterpret_cast<uint32_t*>(to);
  uint32_t* d = reinterpret_cast<uint32_t*>(diff.data());
  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width; x++) {
      if (*f != *t) {
        *d = *t;
        left = std::min(left, x);
        top = std::min(top, y);
        right = std::min(right, width - x - 1);
        bottom = std::min(bottom, height - y - 1);
      }
      ++f;
      ++t;
      ++d;
    }
  }
  if (left == std::numeric_limits<size_t>::max()) {
    return 0;
  }
  const size_t cw = width - left - right;
  const size_t ch = height - top - bottom;
  std::vector<sample_type> cropped(cw * ch * 4);
  for (size_t y = 0; y < ch; y++) {
    memcpy_s(cropped.data() + y * cw * 4, cw * 4, diff.data() + (top + y) * width * 4 + left * 4, cw * 4);
  }
  return calc_png_size(cropped.data(), cw, ch);
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
    png_read_png(png, info, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_STRIP_ALPHA | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_GRAY_TO_RGB, nullptr);
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
    }
  }
  fclose(file);
  png_destroy_read_struct(&png, &info, nullptr);
  return { image, width, height };
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << "usage: scan input1.png input2.png ... [> matrix.txt]" << std::endl;
    return 0;
  }
  char** input_files = argv + 1;
  int num_input_files = argc - 1;
  std::cout << num_input_files << std::endl;
  std::vector<image_type> images(num_input_files);
  for (int i = 0; i < num_input_files; i++) {
    std::cout << input_files[i] << std::endl;
    images[i] = read_png_from_file(input_files[i]);
    if (std::get<0>(images[i]).empty()) {
      std::cerr << "failed to read \"" << input_files[i] << "\"" << std::endl;
      return -1;
    }
    if (std::get<1>(images[i]) != std::get<1>(images[0]) || std::get<2>(images[i]) != std::get<2>(images[0])) {
      std::cerr << "unmatched image size in \"" << input_files[i] << "\"" << std::endl;
      return -1;
    }
  }
  std::vector<std::vector<size_t> > result_matrix(num_input_files);
  for (int i = 0; i < num_input_files; i++) {
    result_matrix[i].resize(num_input_files);
  }
#pragma omp parallel for
  for (int from = 0; from < num_input_files; from++) {
    for (int to = 0; to < num_input_files; to++) {
      if (from == to) {
        result_matrix[from][from] = calc_png_size(std::get<0>(images[from]).data(), std::get<1>(images[from]), std::get<2>(images[from]));
      } else {
        result_matrix[from][to] = calc_diff_size(std::get<0>(images[from]).data(), std::get<0>(images[to]).data(), std::get<1>(images[from]), std::get<2>(images[from]));
      }
    }
  }
  for (int from = 0; from < num_input_files; from++) {
    for (int to = 0; to < num_input_files; to++) {
      std::cout << result_matrix[from][to];
      if (to != num_input_files - 1) {
        std::cout << "\t";
      } else {
        std::cout << std::endl;
      }
    }
  }
}
