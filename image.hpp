#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>
#include <span>
#include <map>

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

    auto unload(void) -> void;
    auto clone(image* img) -> void;
    auto upload(void) -> void;
};

class image_module {
private:
    std::string_view m_name;
    std::vector<std::string_view> m_extensions;
    std::function<void(image*, const char*)> m_func_load;
    std::function<void(image*, const char*)> m_func_store;

public:
    image_module(std::string_view name, std::vector<std::string_view>&& extensions, std::function<void(image*, const char*)>&& func_load, std::function<void(image*, const char*)>&& func_store);

    auto name(void) -> std::string_view;
    auto extensions(void) -> std::span<std::string_view>;
    auto load(image* img, const char* filepath) -> void;
    auto store(image* img, const char* filepath) -> void;
};

auto image_module_register(image_module&& module) -> void;
auto image_module_list(void) -> std::list<npk::image_module>&;
auto image_module_map(void) -> std::map<std::string_view, std::reference_wrapper<npk::image_module>>&;
auto image_module_init(void) -> void;

} // namespace npk
