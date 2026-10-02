#include <imnodes.h>

#include "utils.hpp"
#include "link.hpp"

namespace npk {

link::link(int from_node, int from_attribute, int to_node, int to_attribute)
    : uid(get_uid()), from_node(from_node), from_attribute(from_attribute), to_node(to_node), to_attribute(to_attribute) {}

auto link::render(void) -> void {
    ImNodes::Link(uid, from_attribute, to_attribute);
}

} // namespace npk

