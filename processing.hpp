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
auto min_2d(image* in, image* out) -> void;
auto max_2d(image* in, image* out) -> void;
auto sum_2d(image* in, image* out) -> void;
auto sub_2d(image* in, image* out) -> void;
auto mul_2d(image* in, image* out) -> void;
auto gaussian_2d(image* in, image* out, float space_deviation) -> void;
auto bilateral_2d(image* in, image* out, float space_deviation, float color_deviation) -> void;
auto mean_2d(image* in, image* out, int radius) -> void;
} // namespace npk
