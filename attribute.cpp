#include "utils.hpp"
#include "attribute.hpp"

namespace npk {

attribute::attribute(std::string_view name, std::string_view description)
    : m_uid(get_uid()), m_name(name), m_description(description), m_data(0) {}

auto attribute::name(void) -> std::string_view { return m_name; }
auto attribute::description(void) -> std::string_view { return m_description; }

auto attribute::uid(void) -> int { return m_uid; }
auto attribute::set(void* data) -> void { m_data = data; };
auto attribute::get(void) -> void* { return m_data; }

} // namespace npk

