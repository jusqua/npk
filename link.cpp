#include <imnodes.h>

#include "utils.hpp"
#include "link.hpp"

namespace npk {

link::link(int from_node, int from_attribute, int to_node, int to_attribute)
    : m_uid(get_uid()), m_from_node(from_node), m_from_attribute(from_attribute),
        m_to_node(to_node), m_to_attribute(to_attribute) {}

auto link::render(void) -> void {
    ImNodes::Link(m_uid, m_from_attribute, m_to_attribute);
}

auto link::uid(void) -> int { return m_uid; }
auto link::nodes() -> std::pair<int, int> { return { m_from_node, m_to_node }; }
auto link::attributes() -> std::pair<int, int> { return { m_from_attribute, m_to_attribute }; }

auto link::from_node() -> int { return m_from_node; }
auto link::from_attribute() -> int { return m_from_attribute; }
auto link::to_node() -> int { return m_to_node; }
auto link::to_attribute() -> int { return m_to_attribute; }

auto link::with_node(int uid) -> bool {
    return uid == m_from_node || uid == m_to_node;
}
auto link::with_attribute(int uid) -> bool {
    return uid == m_from_attribute || uid == m_to_attribute;
}
auto link::between_nodes(int from, int to) -> bool {
    return from == m_from_node && to == m_to_node;
}
auto link::between_attributes(int from, int to) -> bool {
    return from == m_from_attribute && to == m_to_attribute;
}


} // namespace npk

