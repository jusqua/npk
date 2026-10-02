#include <hok.hpp>
#include "processing.hpp"

static auto s_queue = sycl::queue{sycl::gpu_selector_v};

template<typename K>
static auto launch_kernel_2d(npk::image* img, K&& kernel) -> void {
    auto buffer = sycl::malloc_device<uint8_t>(img->size, s_queue);
    s_queue.copy(img->data, buffer, img->size).wait();
    s_queue.parallel_for(sycl::range{img->width, img->height}, kernel(buffer)).wait();
    s_queue.copy(buffer, img->data, img->size).wait();
    sycl::free(buffer, s_queue);
}

namespace npk {

auto adjust_intensity_2d(image* img, float factor) -> void {
    launch_kernel_2d(img, [factor](auto buffer) { return hok::intensity<2>(buffer, buffer, factor); });
}

auto adjust_contrast_2d(image* img, float factor) -> void {
    launch_kernel_2d(img, [factor](auto buffer) { return hok::contrast<2>(buffer, buffer, factor); });
}

auto adjust_color_2d(image* img, color& clr) -> void {
    launch_kernel_2d(img, [&clr](auto buffer) { return hok::adjust<2>(buffer, buffer, sycl::float4{clr.r, clr.g, clr.b, clr.a}); });
}

auto convert_gray_2d(image* img) -> void {
    launch_kernel_2d(img, [](auto buffer) { return hok::gray<2>(buffer, buffer); });
}

auto invert_color_2d(image* img) -> void {
    launch_kernel_2d(img, [](auto buffer) { return hok::invert<2>(buffer, buffer); });
}

auto thresholding_2d(image* img, float threshold) -> void {
    launch_kernel_2d(img, [threshold](auto buffer) { return hok::thresh<2>(buffer, buffer, threshold); });
}

auto binarization_2d(image* img, float threshold) -> void {
    launch_kernel_2d(img, [threshold](auto buffer) { return hok::binary<2>(buffer, buffer, threshold); });
}

} // namespace npk
