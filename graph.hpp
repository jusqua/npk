#pragma once

#include <list>
#include <stack>
#include <unordered_map>

#include "node.hpp"
#include "link.hpp"

namespace npk {

class graph {
private:
    bool minimap_enabled;
    bool snapping_enabled;

    std::unordered_map<int, std::list<link>> m_links;
    std::unordered_map<int, bool> m_marked;
    std::unordered_map<int, node> m_nodes;

    auto attach_node(node&& n) -> void;
    auto detach_node(int uid) -> void;
    auto create_link(int from, int to) -> void;
    auto delete_link(int uid) -> void;

    auto find_node_from_attribute(int uid) -> node*;

    auto visit(std::stack<int>& stack, node& n) -> void;
    auto traverse(std::stack<int>& stack) -> void;
    auto evaluate(void) -> void;

public:
    graph();

    auto render(void) -> void;
};

} // namespace npk
