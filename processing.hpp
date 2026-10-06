#pragma once

#include "color.hpp"
#include "image.hpp"

namespace npk {

auto adjust_brightness_2d(image* img, float factor) -> void;
auto adjust_contrast_2d(image* img, float factor) -> void;
auto adjust_color_2d(image* img, color& clr) -> void;
auto convert_gray_2d(image* img) -> void;
auto invert_color_2d(image* img) -> void;
auto thresholding_2d(image* img, float threshold) -> void;
auto binarization_2d(image* img, float threshold) -> void;

} // namespace npk
