#pragma once

#include <string_view>

namespace npk {

class attribute {
private:
    int m_uid;

    std::string_view m_name;
    std::string_view m_description;

    void* m_data;

public:
    attribute(std::string_view name, std::string_view description = "");

    auto uid(void) -> int;

    auto name(void) -> std::string_view;
    auto description(void) -> std::string_view;

    auto set(void* data) -> void;
    auto get(void) -> void*;
};

} // namespace npk
