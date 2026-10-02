#pragma once

namespace npk {

struct link {
    int uid;
    int from_node;
    int from_attribute;
    int to_node;
    int to_attribute;

    link(int from_node, int from_attribute, int to_node, int to_attribute);
    auto render(void) -> void;
};

} // namespace npk

