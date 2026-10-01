#include <print>

#include <hok.hpp>
#include "processing.hpp"

static auto s_queue = sycl::queue{sycl::gpu_selector_v};

namespace npk {

auto adjust_color_2d(image* img, color& clr) -> void {
    std::println("NPK Info: Processing with device {} started", s_queue.get_device().get_info<sycl::info::device::name>());

    auto data = sycl::malloc_device<uint8_t>(img->size, s_queue);
    s_queue.copy(img->data, data, img->size).wait();
    s_queue.parallel_for(
        sycl::range{img->width, img->height},
        hok::adjust<2>(data, data, sycl::float4{clr.r, clr.g, clr.b, clr.a})
    ).wait();
    s_queue.copy(data, img->data, img->size).wait();
    sycl::free(data, s_queue);

    std::println("NPK Info: Processing with device {} ended", s_queue.get_device().get_info<sycl::info::device::name>());
}

} // namespace npk
