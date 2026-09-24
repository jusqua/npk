#pragma once

namespace npk {

struct image {
    int width;
    int height;
    unsigned long size;
    unsigned char color_type;
    unsigned char bit_depth;
    unsigned char* data;
    unsigned int gl_id;

    static auto load(const char* filepath) -> image*;
    static auto store(const char *filename, image* img) -> void;
    auto upload(void) -> void;
};

} // namespace npk
