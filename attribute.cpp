#include "utils.hpp"
#include "attribute.hpp"

namespace npk {

attribute::attribute(attribute_kind kind)
    : uid(get_uid()), kind(kind) {}

} // namespace npk

