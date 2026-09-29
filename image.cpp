#include "image.hpp"

#include <png.h>
#include <GL/gl.h>

#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <print>

static auto libpng_error_callback([[maybe_unused]] struct png_struct_def* def, const char* description) -> void {
    std::println(std::cerr, "LIBPNG Error: {}", description);
}

static auto libpng_warning_callback([[maybe_unused]] struct png_struct_def* def, const char* description) -> void {
    std::println(std::cerr, "LIBPNG Error: {}", description);
}

namespace npk {

auto image::load(const char* filepath) -> void {
    if (data != nullptr) {
        std::println(std::cerr, "NPK Error: Image data must be freed before loading");
        abort();
    }

    auto fd = fopen(filepath, "rb");
    if (!fd) {
        std::println(std::cerr, "NPK Error: Could not open file descriptor for reading PNG");
        abort();
    }

    auto png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, libpng_error_callback, libpng_warning_callback);
    if (!png) {
        fclose(fd);
        std::println(std::cerr, "NPK Error: Could not create structure to read PNG data");
        abort();
    }

    auto info = png_create_info_struct(png);
    if (!info) {
        fclose(fd);
        png_destroy_read_struct(&png, nullptr, nullptr);
        std::println(std::cerr, "NPK Error: Could not create structure to read PNG info");
        abort();
    }

    // Abort if libpng jump back
    if (setjmp(png_jmpbuf(png))) {
        fclose(fd);
        png_destroy_read_struct(&png, &info, nullptr);
        abort();
    };

    png_init_io(png, fd);
    png_read_info(png, info);
    this->width = png_get_image_width(png, info);
    this->height = png_get_image_height(png, info);
    this->color_type = png_get_color_type(png, info);
    this->bit_depth = png_get_bit_depth(png, info);

    if (this->bit_depth == 16) {
        png_set_strip_16(png);
    }

    if (this->color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if (this->color_type == PNG_COLOR_TYPE_GRAY && this->bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }

    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }

    // These color_type don't have an alpha channel then fill it with 0xff.
    if (this->color_type == PNG_COLOR_TYPE_RGB ||
        this->color_type == PNG_COLOR_TYPE_GRAY ||
        this->color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    if (this->color_type == PNG_COLOR_TYPE_GRAY ||
        this->color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    auto row_bytes = png_get_rowbytes(png, info);
    this->channels = 4;
    this->size = this->height * row_bytes;
    this->data = new uint8_t[this->size];
    for(int y = 0; y < this->height; y++) {
        png_read_row(png, (this->data + y * this->width * this->channels * this->bit_depth / 8), nullptr);
    }

    fclose(fd);

    png_destroy_read_struct(&png, &info, NULL);
}

auto image::store(const char *filepath) -> void {
    if (data != nullptr) {
        std::println(std::cerr, "NPK Error: Image data must be loaded before storing");
        abort();
    }

    auto fd = fopen(filepath, "wb");
    if (!fd) {
        std::println(std::cerr, "NPK Error: Could not open file descriptor for writing");
        abort();
    }

    auto png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, libpng_error_callback, libpng_warning_callback);
    if (!png) {
        fclose(fd);
        std::println(std::cerr, "NPK Error: Could not create structure to write PNG data");
        abort();
    }

    auto info = png_create_info_struct(png);
    if (!info) {
        fclose(fd);
        png_destroy_read_struct(&png, nullptr, nullptr);
        std::println(std::cerr, "NPK Error: Could not create structure to write PNG info");
        abort();
    }

    if (setjmp(png_jmpbuf(png))) {
        fclose(fd);
        png_destroy_read_struct(&png, &info, nullptr);
        abort();
    }

    png_init_io(png, fd);

    png_set_IHDR(
        png,
        info,
        this->width, this->height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    for(int y = 0; y < this->height; y++) {
        png_write_row(png, (this->data + y * this->width * this->channels * this->bit_depth / 8));
    }
    png_write_end(png, NULL);

    fclose(fd);

    png_destroy_write_struct(&png, &info);
}

auto image::clone(image* img) -> void {
    if (data != nullptr) {
        std::println(std::cerr, "NPK Error: Image data must be freed before loading");
        abort();
    }

    this->bit_depth = img->bit_depth;
    this->channels = img->channels;
    this->color_type = img->color_type;
    this->height = img->height;
    this->width = img->width;
    this->size = img->size;
    this->data = new uint8_t[this->size];
    for (auto i = 0; i < img->size; i++) {
        this->data[i] = img->data[i];
    }
}

auto image::upload(void) -> void {
    if (texture_id != 0) {
        std::println(std::cerr, "NPK Error: Texture must be freed before uploading");
        abort();
    }

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

image::~image() {
    if (data != nullptr) {
        delete[] data;
    }

    if (texture_id != 0) {
        glDeleteTextures(1, &texture_id);
    }
}

} // namespace npk
