#include <memory>

#include <hok.hpp>

#include "processing.hpp"

static auto s_queue = sycl::queue{sycl::gpu_selector_v};

template<typename T>
struct buffer_delete {
    void operator()(T* ptr) const {
        sycl::free(ptr, s_queue);
    }
};

static auto create_buffer(npk::image* img) -> std::unique_ptr<uint8_t, buffer_delete<uint8_t>> {
    return std::unique_ptr<uint8_t, buffer_delete<uint8_t>>(
        sycl::malloc_shared<uint8_t>(img->size, s_queue),
        buffer_delete<uint8_t>()
    );
}

template<typename K>
static auto launch_kernel_2d(npk::image* img, K&& kernel) -> void {
    auto buffer = create_buffer(img);
    s_queue.parallel_for(sycl::range{img->width, img->height}, kernel(buffer.get())).wait();
}

template<typename K>
static auto launch_kernel_2d(npk::image* in, npk::image* out, K&& kernel) -> void {
    auto in_buffer = create_buffer(in);
    auto out_buffer = create_buffer(out);
    s_queue.parallel_for(sycl::range{in->width, in->height}, kernel(in_buffer.get(), out_buffer.get())).wait();
}

namespace npk {

auto adjust_brightness_2d(image* img, float factor) -> void {
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

auto min_2d(image* in, image* out) -> void {
    launch_kernel_2d(in, out, [](auto in_buffer, auto out_buffer) { return hok::min<2>(in_buffer, out_buffer, out_buffer); });
}

auto max_2d(image* in, image* out) -> void {
    launch_kernel_2d(in, out, [](auto in_buffer, auto out_buffer) { return hok::max<2>(in_buffer, out_buffer, out_buffer); });
}

auto sum_2d(image* in, image* out) -> void {
    launch_kernel_2d(in, out, [](auto in_buffer, auto out_buffer) { return hok::sum<2>(in_buffer, out_buffer, out_buffer); });
}

auto sub_2d(image* in, image* out) -> void {
    launch_kernel_2d(in, out, [](auto in_buffer, auto out_buffer) { return hok::sub<2>(in_buffer, out_buffer, out_buffer); });
}

auto mul_2d(image* in, image* out) -> void {
    launch_kernel_2d(in, out, [](auto in_buffer, auto out_buffer) { return hok::mul<2>(in_buffer, out_buffer, out_buffer); });
}

auto gaussian_2d(image* in, image* out, float space_deviation) -> void {
    launch_kernel_2d(in, out, [space_deviation](auto in_buffer, auto out_buffer) { return hok::gaussian<2>(in_buffer, out_buffer, space_deviation); });
}

auto bilateral_2d(image* in, image* out, float space_deviation, float color_deviation) -> void {
    launch_kernel_2d(in, out, [space_deviation, color_deviation](auto in_buffer, auto out_buffer) { return hok::bilateral<2>(in_buffer, out_buffer, space_deviation, color_deviation); });
}

auto mean_2d(image* in, image* out, int radius) -> void {
    launch_kernel_2d(in, out, [radius](auto in_buffer, auto out_buffer) { return hok::mean<2>(in_buffer, out_buffer, radius); });
}

// TODO: Rework median filter
// auto median_2d(image* in, image* out) -> void {
//     launch_kernel_2d(in, out, [](auto in_buffer, auto out_buffer) { return hok::median<2, 3>(in_buffer, out_buffer); });
// }

} // namespace npk
