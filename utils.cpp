#include "utils.hpp"

static int uid_count = 0;

namespace npk {

auto get_uid(void) -> int {
    return ++uid_count;
}

} // namespace npk
