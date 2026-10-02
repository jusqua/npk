#pragma once

#include <memory>
#include <unordered_map>

#include "node.hpp"
#include "link.hpp"

namespace npk {

struct graph {
    std::unordered_map<int, std::vector<std::shared_ptr<node>>> neighbors;
    std::unordered_map<int, std::shared_ptr<node>> nodes;
    std::unordered_map<int, std::shared_ptr<link>> links;

    auto attach_node(std::shared_ptr<node>&& n);
    auto detach_node(int uid);
    auto create_link(int from, int to);
    auto delete_link(int uid);

    auto visit(std::stack<int>& stack, std::shared_ptr<node>& n) -> void;
    auto traverse(std::stack<int>& stack) -> void;
    auto evaluate(void) -> void;

    auto render(void) -> void;
};

} // namespace npk
