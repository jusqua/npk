#include <memory>

#include <sycl/sycl.hpp>
#include <hok.hpp>

#include "processing.hpp"

static auto s_queue = sycl::queue{sycl::gpu_selector_v};

template<typename T>
class delete_device {
private:
    size_t m_size;
    T* m_data;

public:
    delete_device() : m_data(nullptr), m_size(0)  {}
    delete_device(npk::image& img) : m_data(img.data), m_size(img.size)  {}
    delete_device(std::vector<T>& vec) : m_data(vec.data()), m_size(vec.size()) {}
    delete_device(T* data, size_t size) : m_data(data), m_size(size) {}

    void operator()(T* ptr) const {
        if (m_data != nullptr) s_queue.copy(ptr, m_data, m_size).wait();

        sycl::free(ptr, s_queue);
    }
};

template<bool copy, bool copy_back>
inline static auto create_device(npk::image* img) -> std::unique_ptr<uint8_t, delete_device<uint8_t>> {
    auto buffer = sycl::malloc_device<uint8_t>(img->size, s_queue);
    auto deleter = copy_back ? delete_device<uint8_t>(*img) : delete_device<uint8_t>();
    if (copy) s_queue.copy(img->data, buffer, img->size).wait();
    return std::unique_ptr<uint8_t, delete_device<uint8_t>>(buffer, deleter);
}

template<bool copy, bool copy_back, typename T>
inline static auto create_device(std::vector<T>& vec) -> std::unique_ptr<T, delete_device<T>> {
    auto buffer = sycl::malloc_device<T>(vec.size(), s_queue);
    auto deleter = copy_back ? delete_device<T>(vec) : delete_device<T>();
    if (copy) s_queue.copy(vec.data(), buffer, vec.size()).wait();
    return std::unique_ptr<T, delete_device<T>>(buffer, deleter);
}

template<int dimensions, typename K>
requires std::invocable<K, sycl::item<dimensions>>
inline static auto launch_kernel(sycl::range<dimensions> range, K&& kernel) -> void {
    s_queue.parallel_for(range, kernel).wait();
}

template<int dimensions, typename ...K>
requires (std::invocable<K, sycl::item<dimensions>> && ...)
inline static auto launch_kernel(sycl::range<dimensions> range, std::tuple<K...>&& kernels) -> void {
    std::apply([&](auto&... kernel) { (s_queue.parallel_for(range, kernel).wait(), ...); }, kernels);
}

template<typename K>
inline static auto inline_launch_kernel_2d(npk::image* img, K&& kernel) -> void {
    auto d_img = create_device<true, true>(img);
    launch_kernel(sycl::range{img->height, img->width}, kernel(d_img.get()));
}

template<typename K>
inline static auto inline_launch_kernel_2d(npk::image* in, npk::image* out, K&& kernel) -> void {
    auto d_in = create_device<true, false>(in);
    auto d_out = create_device<true, true>(out);
    launch_kernel(sycl::range{in->height, in->width}, kernel(d_in.get(), d_out.get()));
}

template<typename K>
inline static auto launch_kernel_2d(npk::image* in, npk::image* out, K&& kernel) -> void {
    auto d_in = create_device<true, false>(in);
    auto d_out = create_device<false, true>(out);
    launch_kernel(sycl::range{in->height, in->width}, kernel(d_in.get(), d_out.get()));
}

template<typename T, typename K>
inline static auto launch_kernel_2d(npk::image* in, npk::image* out, std::vector<T>& filter, K&& kernel) -> void {
    auto d_in = create_device<true, false>(in);
    auto d_out = create_device<false, true>(out);
    auto d_filter = create_device<true, false>(filter);
    launch_kernel(sycl::range{in->height, in->width}, kernel(d_in.get(), d_out.get(), d_filter.get()));
}

template<typename T, typename K>
inline static auto launch_buffered_kernel_2d(npk::image* in, npk::image* out, std::vector<T>& filter, K&& kernel) -> void {
    auto d_in = create_device<true, false>(in);
    auto d_out = create_device<false, true>(out);
    auto d_buffer = create_device<false, false>(out);
    auto d_filter = create_device<true, false>(filter);
    launch_kernel(sycl::range{in->height, in->width}, kernel(d_in.get(), d_out.get(), d_buffer.get(), d_filter.get()));
}

namespace npk {

auto adjust_brightness_2d(image* img, float factor) -> void {
    inline_launch_kernel_2d(img, [factor](auto d_img) {
        return hok::intensity<2>(d_img, d_img, factor);
    });
}

auto adjust_contrast_2d(image* img, float factor) -> void {
    inline_launch_kernel_2d(img, [factor](auto d_img) {
        return hok::contrast<2>(d_img, d_img, factor);
    });
}

auto adjust_color_2d(image* img, color& clr) -> void {
    inline_launch_kernel_2d(img, [&clr](auto d_img) {
        return hok::adjust<2>(d_img, d_img, sycl::float4{clr.r, clr.g, clr.b, clr.a});
    });
}

auto convert_gray_2d(image* img) -> void {
    inline_launch_kernel_2d(img, [](auto d_img) {
        return hok::gray<2>(d_img, d_img);
    });
}

auto invert_color_2d(image* img) -> void {
    inline_launch_kernel_2d(img, [](auto d_img) {
        return hok::invert<2>(d_img, d_img);
    });
}

auto thresholding_2d(image* img, float threshold) -> void {
    inline_launch_kernel_2d(img, [threshold](auto d_img) {
        return hok::thresh<2>(d_img, d_img, threshold);
    });
}

auto binarization_2d(image* img, float threshold) -> void {
    inline_launch_kernel_2d(img, [threshold](auto d_img) {
        return hok::binary<2>(d_img, d_img, threshold);
    });
}

auto min_2d(image* in, image* out) -> void {
    inline_launch_kernel_2d(in, out, [](auto d_in, auto d_out) {
        return hok::min<2>(d_in, d_out, d_out);
    });
}

auto max_2d(image* in, image* out) -> void {
    inline_launch_kernel_2d(in, out, [](auto d_in, auto d_out) {
        return hok::max<2>(d_in, d_out, d_out);
    });
}

auto sum_2d(image* in, image* out) -> void {
    inline_launch_kernel_2d(in, out, [](auto d_in, auto d_out) {
        return hok::sum<2>(d_in, d_out, d_out);
    });
}

auto sub_2d(image* in, image* out) -> void {
    inline_launch_kernel_2d(in, out, [](auto d_in, auto d_out) {
        return hok::sub<2>(d_in, d_out, d_out);
    });
}

auto mul_2d(image* in, image* out) -> void {
    inline_launch_kernel_2d(in, out, [](auto d_in, auto d_out) {
        return hok::mul<2>(d_in, d_out, d_out);
    });
}

auto gaussian_2d(image* in, image* out, float space_deviation) -> void {
    launch_kernel_2d(in, out, [space_deviation](auto d_in, auto d_out) {
        return hok::gaussian<2>(d_in, d_out, space_deviation);
    });
}

auto bilateral_2d(image* in, image* out, float space_deviation, float color_deviation) -> void {
    launch_kernel_2d(in, out, [space_deviation, color_deviation](auto d_in, auto d_out) {
        return hok::bilateral<2>(d_in, d_out, space_deviation, color_deviation);
    });
}

auto mean_2d(image* in, image* out, int radius) -> void {
    launch_kernel_2d(in, out, [radius](auto d_in, auto d_out) {
        return hok::mean<2>(d_in, d_out, radius);
    });
}

// TODO: Rework median filter
// auto median_2d(image* in, image* out) -> void {
//     launch_kernel_2d(in, out, [](auto d_in, auto d_out) {
//         return hok::median<2, 3>(d_in, d_out);
//     });
// }

auto convolve_2d(image* in, image* out, std::vector<float>& filter, std::vector<int>& shape) -> void {
    launch_kernel_2d(in, out, filter, [&shape](auto d_in, auto d_out, auto d_filter) {
        return hok::convolve(d_in, d_out, sycl::range(shape[1], shape[0]), d_filter);
    });
}

auto dilate_2d(image* in, image* out, std::vector<uint8_t>& strel, std::vector<int>& shape) -> void {
    launch_kernel_2d(in, out, strel, [&shape](auto d_in, auto d_out, auto d_strel) {
        return hok::dilate(d_in, d_out, sycl::range(shape[1], shape[0]), reinterpret_cast<bool*>(d_strel));
    });
}

auto erode_2d(image* in, image* out, std::vector<uint8_t>& strel, std::vector<int>& shape) -> void {
    launch_kernel_2d(in, out, strel, [&shape](auto d_in, auto d_out, auto d_strel) {
        return hok::erode(d_in, d_out, sycl::range(shape[1], shape[0]), reinterpret_cast<bool*>(d_strel));
    });
}

auto open_2d(image* in, image* out, std::vector<uint8_t>& strel, std::vector<int>& shape) -> void {
    launch_buffered_kernel_2d(in, out, strel, [&shape](auto d_in, auto d_out, auto d_buffer, auto d_strel) {
        return hok::open(d_in, d_out, d_buffer, sycl::range(shape[1], shape[0]), reinterpret_cast<bool*>(d_strel));
    });
}

auto close_2d(image* in, image* out, std::vector<uint8_t>& strel, std::vector<int>& shape) -> void {
    launch_buffered_kernel_2d(in, out, strel, [&shape](auto d_in, auto d_out, auto d_buffer, auto d_strel) {
        return hok::close(d_in, d_out, d_buffer, sycl::range(shape[1], shape[0]), reinterpret_cast<bool*>(d_strel));
    });
}

auto white_tophat_2d(image* in, image* out, std::vector<uint8_t>& strel, std::vector<int>& shape) -> void {
    launch_buffered_kernel_2d(in, out, strel, [&shape](auto d_in, auto d_out, auto d_buffer, auto d_strel) {
        return hok::white_tophat(d_in, d_out, d_buffer, sycl::range(shape[1], shape[0]), reinterpret_cast<bool*>(d_strel));
    });
}

auto black_tophat_2d(image* in, image* out, std::vector<uint8_t>& strel, std::vector<int>& shape) -> void {
    launch_buffered_kernel_2d(in, out, strel, [&shape](auto d_in, auto d_out, auto d_buffer, auto d_strel) {
        return hok::black_tophat(d_in, d_out, d_buffer, sycl::range(shape[1], shape[0]), reinterpret_cast<bool*>(d_strel));
    });
}

} // namespace npk
