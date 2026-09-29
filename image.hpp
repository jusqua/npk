#pragma once

#include <cstdint>

namespace npk {

struct image {
    uint32_t width = 0;
    uint32_t height = 0;
    uint64_t size = 0;
    uint8_t color_type = 0;
    uint8_t bit_depth = 0;
    uint8_t channels = 0;
    uint32_t texture_id = 0;
    uint8_t* data = nullptr;

    ~image();

    auto load(const char* filepath) -> void;
    auto store(const char* filepath) -> void;
    auto clone(image* img) -> void;
    auto upload(void) -> void;
};

} // namespace npk
