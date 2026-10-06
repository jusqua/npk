#pragma once

#include <utility>
namespace npk {

class link {
private:
    int m_uid;
    int m_from_node;
    int m_from_attribute;
    int m_to_node;
    int m_to_attribute;

public:
    link(int from_node, int from_attribute, int to_node, int to_attribute);

    auto uid(void) -> int;
    auto nodes() -> std::pair<int, int>;
    auto attributes() -> std::pair<int, int>;

    auto from_node() -> int;
    auto from_attribute() -> int;
    auto to_node() -> int;
    auto to_attribute() -> int;

    auto with_node(int uid) -> bool;
    auto with_attribute(int uid) -> bool;
    auto between_nodes(int from, int to) -> bool;
    auto between_attributes(int from, int to) -> bool;

    auto render(void) -> void;
};

} // namespace npk

