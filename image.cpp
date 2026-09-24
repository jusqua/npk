#include "image.hpp"

#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <png.h>
#include <GL/gl.h>

#include <iostream>
#include <print>

static auto libpng_error_callback([[maybe_unused]] struct png_struct_def* def, const char* description) -> void {
    std::println(std::cerr, "LIBPNG Error: {}", description);
}

namespace npk {

auto image::load(const char* filepath) -> image* {
    auto img = new image();
    auto fd = fopen(filepath, "rb");

    auto png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, libpng_error_callback, nullptr);
    if (!png) {
        std::println(std::cerr, "NPK Error: Failed to create PNG read structure");
        abort();
    }

    auto info = png_create_info_struct(png);
    if (!info) {
        std::println(std::cerr, "NPK Error: Failed to create PNG info structure");
        abort();
    }

    // Abort if libpng jump back
    if (setjmp(png_jmpbuf(png))) {
        abort();
    };

    png_init_io(png, fd);
    png_read_info(png, info);
    img->width = png_get_image_width(png, info);
    img->height = png_get_image_height(png, info);
    img->color_type = png_get_color_type(png, info);
    img->bit_depth = png_get_bit_depth(png, info);

    if (img->bit_depth == 16) {
        png_set_strip_16(png);
    }

    if (img->color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if (img->color_type == PNG_COLOR_TYPE_GRAY && img->bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }

    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }

    // These color_type don't have an alpha channel then fill it with 0xff.
    if (img->color_type == PNG_COLOR_TYPE_RGB ||
        img->color_type == PNG_COLOR_TYPE_GRAY ||
        img->color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    if (img->color_type == PNG_COLOR_TYPE_GRAY ||
        img->color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    auto row_bytes = png_get_rowbytes(png, info);
    img->size = img->height * row_bytes;
    img->data = new uint8_t[img->size];
    for(int y = 0; y < img->height; y++) {
        png_read_row(png, (img->data + y * img->width * 4 * img->bit_depth / 8), nullptr);
    }

    fclose(fd);

    png_destroy_read_struct(&png, &info, NULL);
    return img;
}

auto image::store(const char *filename, image* img) -> void {
  int y;

  FILE *fp = fopen(filename, "wb");
  if(!fp) abort();

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) abort();

  png_infop info = png_create_info_struct(png);
  if (!info) abort();

  if (setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  png_set_IHDR(
    png,
    info,
    img->width, img->height,
    8,
    PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(png, info);

  for(int y = 0; y < img->height; y++) {
      png_write_row(png, (img->data + y * img->width * 4 * img->bit_depth / 8));
  }
  png_write_end(png, NULL);

  fclose(fp);

  png_destroy_write_struct(&png, &info);
}

auto image::upload(void) -> void {
    glGenTextures(1, &gl_id);
    glBindTexture(GL_TEXTURE_2D, gl_id);

    glTexParameteri(gl_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(gl_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(gl_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(gl_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace npk
