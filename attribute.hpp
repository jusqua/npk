#pragma once

#include <string>

namespace npk {

enum struct attribute_kind {
    image,
    color,
    value,
};

struct attribute {
    int uid;
    std::string description;
    attribute_kind kind;

    attribute(attribute_kind kind);
};

} // namespace npk
