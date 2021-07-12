
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

void write_diff_png_to_file(sample_type* from, sample_type* to, width_type width, height_type height, FILE* fp, size_t* offset_x, size_t* offset_y) {
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
    *offset_x = 0;
    *offset_y = 0;
    return;
  }
  const size_t cw = width - left - right;
  const size_t ch = height - top - bottom;
  std::vector<sample_type> cropped(cw * ch * 4);
  for (size_t y = 0; y < ch; y++) {
    memcpy_s(cropped.data() + y * cw * 4, cw * 4, diff.data() + (top + y) * width * 4 + left * 4, cw * 4);
  }
  write_png_to_file(cropped.data(), cw, ch, fp);
  *offset_x = left;
  *offset_y = top;
  return;
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

void print_usage() {
  std::cout << "usage: organize -s solution.txt [-o output_dir] matrix.txt" << std::endl;
}

std::vector<size_t> load_solution(std::ifstream& solution, size_t N) {
  std::vector<size_t> arcs(N);  // arc[j] = i (arc from i to j)
  std::vector<size_t> costs(N);
  std::string line;
  std::regex re(R"(X\[\d+,(\d+),(\d+)\] +1 +(\d+))");
  std::smatch m;
  while (std::getline(solution, line)) {
    if (std::regex_search(line, m, re)) {
      size_t from = strtoul(m[1].str().c_str(), nullptr, 10);
      size_t to = strtoul(m[2].str().c_str(), nullptr, 10);
      size_t cost = strtoul(m[3].str().c_str(), nullptr, 10);
      if (arcs[to] <= cost) {
        arcs[to] = from;
        costs[to] = cost;
      }
    }
  }
  return arcs;
}

int main(int argc, char** argv) {
  std::string solution_filename;
  std::string graph_filename;
  std::string output_dirname("output");
  if (argc < 4) {
    print_usage();
    return 0;
  }
  int i = 1;
  while (i < argc) {
    if (strcmp(argv[i], "-s") == 0) {
      ++i;
      if (i >= argc) {
        print_usage();
        return 0;
      }
      solution_filename = argv[i];
      ++i;
    } else if (strcmp(argv[i], "-o") == 0) {
      ++i;
      if (i >= argc) {
        print_usage();
        return 0;
      }
      output_dirname = argv[i];
      ++i;
    } else {
      graph_filename = argv[i];
      ++i;
    }
  }
  if (solution_filename.empty() || graph_filename.empty()) {
    print_usage();
    return 0;
  }
  std::ifstream graph(graph_filename);
  if (!graph) {
    std::cerr << "failed to read \"" << graph_filename << "\"" << std::endl;
    return -1;
  }
  size_t N;
  graph >> N;
  std::vector<std::string> files(N);
  std::vector<std::string> basenames(N);
  std::vector<image_type> images(N);
  for (size_t i = 0; i < N; ++i) {
    graph >> files[i];
    const auto delim = files[i].find_last_of("/\\");
    const auto offset = (delim == std::string::npos) ? 0 : delim + 1;
    const auto ext = files[i].find_last_of(".");
    const auto count = (ext == std::string::npos || ext < offset) ? std::string::npos : ext - offset;
    basenames[i] = files[i].substr(offset, count);
    images[i] = read_png_from_file(files[i].c_str());
    if (std::get<0>(images[i]).empty()) {
      std::cerr << "failed to read \"" << files[i] << "\"" << std::endl;
      return -1;
    }
    if (std::get<1>(images[i]) != std::get<1>(images[0]) || std::get<2>(images[i]) != std::get<2>(images[0])) {
      std::cerr << "unmatched image size in \"" << files[i] << "\"" << std::endl;
      return -1;
    }
  }
  std::filesystem::create_directory(output_dirname);
  std::ifstream solution(solution_filename);
  if (!solution) {
    std::cerr << "failed to read \"" << solution_filename << "\"" << std::endl;
    return -1;
  }
  auto arcs = load_solution(solution, N);
  for (size_t i = 0; i < N; ++i) {
    if (arcs[i] == i) {
      const auto filename_png = output_dirname + "/" + basenames[i] + ".png";
      const auto filename_stir = output_dirname + "/" + basenames[i] + ".stir";
      FILE* fp;
      if (fopen_s(&fp, filename_png.c_str(), "wb") || !fp) {
        std::cerr << "failed to write \"" << filename_png << "\"" << std::endl;
        return -1;
      }
      write_png_to_file(std::get<0>(images[i]).data(), std::get<1>(images[i]), std::get<2>(images[i]), fp);
      fclose(fp);
      std::ofstream metadata(filename_stir);
      if (!metadata) {
        std::cerr << "failed to write \"" << filename_stir << "\"" << std::endl;
        return -1;
      }
      metadata << basenames[i] << ".png" << std::endl;
    } else {
      const auto filename_png = output_dirname + "/" + basenames[i] + ".png";
      const auto filename_stir = output_dirname + "/" + basenames[i] + ".stir";
      FILE* fp;
      if (fopen_s(&fp, filename_png.c_str(), "wb") || !fp) {
        std::cerr << "failed to write \"" << filename_png << "\"" << std::endl;
        return -1;
      }
      size_t offset_x, offset_y;
      write_diff_png_to_file(std::get<0>(images[arcs[i]]).data(),
                             std::get<0>(images[i]).data(),
                             std::get<1>(images[i]), std::get<2>(images[i]),
                             fp, &offset_x, &offset_y);
      fclose(fp);
      std::ofstream metadata(filename_stir);
      if (!metadata) {
        std::cerr << "failed to write \"" << filename_stir << "\"" << std::endl;
        return -1;
      }
      metadata << basenames[i] << ".png" << std::endl;
      metadata << basenames[arcs[i]] << ".stir" << std::endl;
      metadata << offset_x << std::endl << offset_y << std::endl;
    }
  }
}
