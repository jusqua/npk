#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <functional>
#include <list>
#include <filesystem>
#include <iostream>
#include <print>

#include <png.h>
#include <tiffio.h>
#include <GLFW/glfw3.h>

#include "image.hpp"
#include "tiff.h"

namespace fs = std::filesystem;


static std::list<npk::image_module> s_image_module_list;
static std::map<std::string_view, std::reference_wrapper<npk::image_module>> s_image_module_map;

static auto get_module_from_filepath(const char* filepath) -> npk::image_module& {
    auto path = fs::path(filepath);
    if (!path.has_extension()) {
        std::println(std::cerr, "NPK Error: File \"{}\" has no extension", filepath);
        abort();
    }

    auto extension = path.extension().string();
    if (!s_image_module_map.contains(extension)) {
        std::println(std::cerr, "NPK Error: No module for extension {} available for file {}", extension, filepath);
        abort();
    }
    return s_image_module_map.at(extension).get();
}

namespace npk {

auto image::load(const char* filepath) -> void {
    if (data != nullptr) {
        std::println(std::cerr, "NPK Error: Image data must be freed before loading");
        abort();
    }

    get_module_from_filepath(filepath).load(this, filepath);
}

auto image::store(const char *filepath) -> void {
    if (data == nullptr) {
        std::println(std::cerr, "NPK Error: Image data must be loaded before storing");
        abort();
    }

    get_module_from_filepath(filepath).store(this, filepath);
}

auto image::unload(void) -> void {
    if (this->data != nullptr)
        delete[] this->data;
    if (this->texture_id != 0)
        glDeleteTextures(1, &texture_id);
    this->bit_depth = 0;
    this->channels = 0;
    this->height = 0;
    this->width = 0;
    this->size = 0;
    this->texture_id = 0;
    this->data = nullptr;
}

auto image::similar(image* img) -> void {
    if (data != nullptr) {
        std::println(std::cerr, "NPK Error: Image data must be freed before loading");
        abort();
    }

    this->bit_depth = img->bit_depth;
    this->channels = img->channels;
    this->height = img->height;
    this->width = img->width;
    this->size = img->size;
    this->texture_id = 0;
    this->data = new uint8_t[this->size];
}

auto image::clone(image* img) -> void {
    similar(img);

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

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

image::~image() {
    unload();
}

image_module::image_module(std::string_view name, std::vector<std::string_view>&& extensions, std::function<void(image*, const char*)>&& func_load, std::function<void(image*, const char*)>&& func_store)
    : m_name(name), m_extensions(extensions), m_func_load(func_load), m_func_store(func_store) {}
auto image_module::name(void) -> std::string_view { return m_name; }
auto image_module::extensions(void) -> std::span<std::string_view> { return m_extensions; }
auto image_module::load(image* img, const char* filepath) -> void { m_func_load(img, filepath); }
auto image_module::store(image* img, const char* filepath) -> void { m_func_store(img, filepath); }

auto image_module_register(image_module&& module) -> void {
    s_image_module_list.insert(s_image_module_list.end(), std::move(module));
    auto& current = s_image_module_list.back();
    for (auto& extension : current.extensions()) {
        s_image_module_map.insert(std::make_pair(extension, std::ref(current)));
    }
}

auto image_module_list(void) -> std::list<npk::image_module>& {
    return s_image_module_list;
}

auto image_module_map(void) -> std::map<std::string_view, std::reference_wrapper<npk::image_module>>& {
    return s_image_module_map;
}

auto image_module_init(void) -> void {
    image_module_register({
        "Portable Network Graphics",
        {".png"},
        [](image* img, const char* filepath) {
            auto fd = fopen(filepath, "rb");
            if (!fd) {
                std::println(std::cerr, "NPK Error: Could not open file descriptor for reading PNG");
                abort();
            }

            auto png = png_create_read_struct(
                PNG_LIBPNG_VER_STRING,
                nullptr,
                []([[maybe_unused]] struct png_struct_def* def, const char* description) -> void {
                    std::println(std::cerr, "LIBPNG Error: {}", description);
                },
                []([[maybe_unused]] struct png_struct_def* def, const char* description) -> void {
                    std::println(std::cerr, "LIBPNG Warning: {}", description);
                }
            );

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
            img->width = png_get_image_width(png, info);
            img->height = png_get_image_height(png, info);
            img->bit_depth = png_get_bit_depth(png, info);
            auto color_type = png_get_color_type(png, info);

            if (img->bit_depth == 16) {
                png_set_strip_16(png);
            }

            if (color_type == PNG_COLOR_TYPE_PALETTE) {
                png_set_palette_to_rgb(png);
            }

            // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
            if (color_type == PNG_COLOR_TYPE_GRAY && img->bit_depth < 8) {
                png_set_expand_gray_1_2_4_to_8(png);
            }

            if (png_get_valid(png, info, PNG_INFO_tRNS)) {
                png_set_tRNS_to_alpha(png);
            }

            // These color_type don't have an alpha channel then fill it with 0xff.
            if (color_type == PNG_COLOR_TYPE_RGB ||
                color_type == PNG_COLOR_TYPE_GRAY ||
                color_type == PNG_COLOR_TYPE_PALETTE) {
                png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
            }

            if (color_type == PNG_COLOR_TYPE_GRAY ||
                color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
                png_set_gray_to_rgb(png);
            }

            png_read_update_info(png, info);

            auto row_bytes = png_get_rowbytes(png, info);
            img->channels = 4;
            img->size = img->height * row_bytes;
            img->data = new uint8_t[img->size];
            for(int y = 0; y < img->height; y++) {
                png_read_row(png, (img->data + y * img->width * img->channels * img->bit_depth / 8), nullptr);
            }

            fclose(fd);

            png_destroy_read_struct(&png, &info, NULL);
        },
        [](image* img, const char* filepath) {
            auto fd = fopen(filepath, "wb");
            if (!fd) {
                std::println(std::cerr, "NPK Error: Could not open file descriptor for writing");
                abort();
            }

            auto png = png_create_write_struct(
                PNG_LIBPNG_VER_STRING,
                nullptr,
                []([[maybe_unused]] struct png_struct_def* def, const char* description) -> void {
                    std::println(std::cerr, "LIBPNG Error: {}", description);
                },
                []([[maybe_unused]] struct png_struct_def* def, const char* description) -> void {
                    std::println(std::cerr, "LIBPNG Warning: {}", description);
                }
            );
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
                img->width, img->height,
                8,
                PNG_COLOR_TYPE_RGBA,
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT
            );
            png_write_info(png, info);

            for(int y = 0; y < img->height; y++) {
                png_write_row(png, (img->data + y * img->width * img->channels * img->bit_depth / 8));
            }
            png_write_end(png, NULL);

            fclose(fd);

            png_destroy_write_struct(&png, &info);
        }
    });
    image_module_register({
        "Tagged Image File Format",
        {".tiff", ".tif"},
        [](image* img, const char* filepath) {
            auto tif = TIFFOpen(filepath, "r");
            if (!tif) {
                std::println(std::cerr, "NPK Error: Could not open file descriptor for reading TIFF");
                abort();
            }

            uint32_t width;
            uint32_t height;
            uint32_t* raster;
            uint64_t size;
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
            size = width * height;
            raster = static_cast<uint32_t*>(_TIFFmalloc(size * sizeof(uint32_t)));
            if (!TIFFReadRGBAImage(tif, width, height, raster, 0)) {
                std::println(std::cerr, "NPK Error: Could not open TIFF file");
                abort();
            }

            TIFFClose(tif);
            img->bit_depth = 8;
            img->channels = 4;
            img->height = height;
            img->width = width;
            img->size = size * img->channels;
            img->texture_id = 0;
            img->data = reinterpret_cast<uint8_t*>(raster);
        },
        [](image* img, const char* filepath) {
            auto tif = TIFFOpen(filepath, "w");

            TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, img->width);
            TIFFSetField(tif, TIFFTAG_IMAGELENGTH, img->height);
            TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, img->channels);
            TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, img->bit_depth);
            TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
            TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

            tsize_t linebytes = img->channels * img->width;

            auto buf = static_cast<uint8_t*>(TIFFScanlineSize(tif)
                ? _TIFFmalloc(linebytes)
                : _TIFFmalloc(TIFFScanlineSize(tif)));

            TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, linebytes));

            for (auto row = 0; row < img->height; row++) {
                memcpy(buf, &img->data[(img->height-row-1)*linebytes], linebytes);
                if (TIFFWriteScanline(tif, buf, row, 0) < 0)
                    break;
            }

            if (buf) _TIFFfree(buf);
            TIFFClose(tif);
        }
    });
}

} // namespace npk
