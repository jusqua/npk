#include "color.hpp"
#include <algorithm>

namespace npk {

color::color() : r(1.f), g(1.f), b(1.f), a(1.f) {}

color::color(float r, float g, float b, float a) {
    this->r = std::clamp(0.f, r, 1.f);
    this->g = std::clamp(0.f, g, 1.f);
    this->b = std::clamp(0.f, b, 1.f);
    this->a = std::clamp(0.f, a, 1.f);
}

} // namespace npk
