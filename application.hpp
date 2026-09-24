#pragma once

#include <functional>

namespace npk {

class application {
public:
    application();
    ~application();

    auto render(std::function<void(void)>&& func) -> void;
};

} // namespace npk
