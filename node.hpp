#pragma once

#include <filesystem>
#include <memory>
#include <stack>
#include <vector>

#include "attribute.hpp"
#include "color.hpp"
#include "image.hpp"

namespace npk {

enum struct node_kind {
    image,
    color,
    value,
    adjust_intensity,
    adjust_contrast,
    adjust_color,
    convert_gray,
    invert_color,
    thresholding,
    binarization,
    display_image,
};

enum struct node_mark {
    tree,
    back,
    forward
};

struct node_data {
    std::filesystem::path path;
    std::shared_ptr<image> img;
    color clr;
    float value;
};

struct node {
    int uid;
    std::string_view title;
    node_kind kind;
    node_mark mark;
    node_data data;
    std::vector<std::shared_ptr<attribute>> inattributes;
    std::vector<std::shared_ptr<attribute>> outattributes;

    node(node_kind kind);

    auto render(void) -> void;
    auto evaluate(std::stack<node_data*>& stack) -> void;
};

} // namespace npk

