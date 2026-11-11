#include <algorithm>
#include <array>
#include <format>
#include <functional>
#include <map>
#include <numeric>
#include <ranges>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>
#include <SDL3/SDL.h>

#include "processing.hpp"
#include "color.hpp"
#include "utils.hpp"
#include "image.hpp"
#include "node.hpp"

static std::map<std::string, std::list<npk::node_module>> s_node_modules;

namespace npk {

node::node(std::string_view name, std::string_view description, std::string_view category, std::any&& intrinsic,
        std::function<std::span<attribute>(node*)>&& func_attibutes, std::function<void(node*)>&& func_render, std::function<void(node*)>&& func_evaluate)
    : m_uid(get_uid()), m_name(name), m_description(description), m_category(category), m_intrinsic(intrinsic),
        m_func_attributes(func_attibutes), m_func_render(func_render), m_func_evaluate(func_evaluate) {}

auto node::uid(void) -> int { return m_uid; }
auto node::name(void) -> std::string_view { return m_name; }
auto node::description(void) -> std::string_view { return m_description; }
auto node::category(void) -> std::string_view { return m_category; }

auto node::intrinsic(void) -> std::any& { return m_intrinsic; }
auto node::attributes(void) -> std::span<attribute> { return m_func_attributes(this); }
auto node::render(void) -> void { return m_func_render(this); }
auto node::evaluate(void) -> void { return m_func_evaluate(this); }

node_module::node_module(std::string_view name, std::string_view description, std::string_view category, std::function<node(node_module*)>&& func_instantiate)
    : m_name(name), m_description(description), m_category(category), m_func_instantiate(func_instantiate) {}

auto node_module::name() -> std::string_view { return m_name; }
auto node_module::description() -> std::string_view { return m_description; }
auto node_module::category() -> std::string_view { return m_category; }
auto node_module::instantiate() -> node { return m_func_instantiate(this); }

auto node_module_register(node_module&& module) -> void {
    auto category = module.category().data();
    if (!s_node_modules.contains(category)) {
        s_node_modules.insert({category, {}});
    }

    s_node_modules.at(category).push_back(std::move(module));
}

auto node_module_map(void) -> std::map<std::string, std::list<npk::node_module>>& {
    return s_node_modules;
}

// TODO: Split node modules into different files
// TODO: Reduce the node module boilerplate
// INFO: Yucch! Too much copy pasting...
auto node_module_init(void) -> void {
    auto current_category = std::string_view("Selectors");

    node_module_register({
        "Image",
        "Select an image from system",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, npk::attribute>;
            auto img = image();
            auto attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(&attr, 1);
            };

            auto render = [](node* self) -> void {
                auto& [img, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                if (ImGui::Button("^ Open image file", {0, 16})) {
                    auto& modules = image_module_list();

                    auto extensions = std::vector<std::string>();
                    extensions.reserve(modules.size() + 1);
                    auto filters = std::vector<SDL_DialogFileFilter>{};
                    filters.reserve(modules.size() + 1);

                    filters.emplace_back("Supported image files", nullptr);
                    for (auto& module : modules) {
                        auto extension = module.extensions()
                            | std::views::transform([](auto&& str) { return str | std::views::drop(1); })
                            | std::views::join_with(';')
                            | std::ranges::to<std::string>();
                        filters.emplace_back(module.name().data(), extensions.emplace_back(std::move(extension)).c_str());
                    }
                    extensions.emplace_back(extensions | std::views::join_with(';') | std::ranges::to<std::string>());
                    filters.front().pattern = extensions.back().c_str();

                    SDL_ShowOpenFileDialog(
                        [](void* userdata, const char* const* filelist, int filter) {
                            if (!filelist || !*filelist || filter < 0) return;

                            auto img = reinterpret_cast<image*>(userdata);
                            img->unload();
                            img->load(*filelist);
                            img->upload();
                        },
                        &img,
                        nullptr,
                        filters.data(),
                        filters.size(),
                        nullptr,
                        false
                    );
                }

                ImNodes::BeginOutputAttribute(attr.uid());
                constexpr auto base = 256;
                auto width = base;
                auto height = base;
                if (img.width > img.height) width = img.width * base / img.height;
                if (img.height > img.width) height = img.height * base / img.width;
                ImGui::Image(img.texture, ImVec2(width, height));
                ImNodes::EndOutputAttribute();

                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), std::move(attr)),
                std::move(attributes),
                std::move(render),
                std::move(evaluate)
            );
        }
    });

    node_module_register({
        "Color",
        "Select a RGBA color",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::color, npk::attribute>;
            auto clr = color();
            auto attr = attribute("Color");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_clr, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(&attr, 1);
            };

            auto render = [](node* self) -> void {
                auto& [clr, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImNodes::BeginOutputAttribute(attr.uid());
                ImGui::PushItemWidth(256);
                ImGui::ColorPicker4("", reinterpret_cast<float*>(&clr), ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview);
                ImGui::PopItemWidth();
                ImNodes::EndOutputAttribute();

                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [clr, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                attr.set(&clr);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(clr), std::move(attr)),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Value",
        "Select a float value",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<float, npk::attribute>;
            auto val = 0.f;
            auto attr = attribute("Value");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_val, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(&attr, 1);
            };

            auto render = [](node* self) -> void {
                auto& [val, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImNodes::BeginOutputAttribute(attr.uid());
                ImGui::PushItemWidth(96);
                ImGui::InputFloat("", &val, 1.f, 1.f);
                ImGui::PopItemWidth();
                ImNodes::EndOutputAttribute();

                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [clr, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                attr.set(&clr);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(val), std::move(attr)),
                attributes,
                render,
                evaluate
            );
        }
    });

    // TODO: Add some presets and 1D option for linear filters
    node_module_register({
        "Filter",
        "Select weights for a convolution filter",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<std::pair<std::vector<float>, std::vector<int>>, npk::attribute>;
            auto val = std::make_pair(std::vector(9, 1.0f/9), std::vector{3, 3});
            auto attr = attribute("Value");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_val, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(&attr, 1);
            };

            auto render = [](node* self) -> void {
                auto& [val, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [data, shape] = val;
                auto id = std::format("node_table_{}", self->uid());
                auto shape_sum = std::accumulate(shape.begin(), shape.end(), 0);

                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImNodes::BeginOutputAttribute(attr.uid());
                ImGui::BeginGroup();
                ImGui::PushItemWidth(96*3);
                for (auto i = 0; i < shape.size(); i++) {
                    ImGui::PushID(i);
                    ImGui::SliderInt(std::format("D{}", i).c_str(), &shape[i], 3, 48);
                    ImGui::PopID();
                }
                ImGui::PopItemWidth();
                ImGui::EndGroup();
                if (ImGui::BeginTable(id.c_str(), shape[1], 0, {96.f*shape[1], 0})) {
                    for (auto i = 0; i < data.size(); i++) {
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(96);
                        ImGui::PushID(i);
                        ImGui::InputFloat("", &data[i], 0, 0, "%.6f");
                        ImGui::PopID();
                        ImGui::PopItemWidth();
                    }
                    ImGui::EndTable();
                }
                ImNodes::EndOutputAttribute();
                ImNodes::EndNode();

                if (std::accumulate(shape.begin(), shape.end(), 0) != shape_sum) {
                    data.resize(std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>()));
                    std::fill(data.begin(), data.end(), 1.0f/data.size());
                }
            };

            auto evaluate = [](node* self) -> void {
                auto& [val, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                attr.set(&val);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(val), std::move(attr)),
                attributes,
                render,
                evaluate
            );
        }
    });

    // TODO: Add some presets and 1D option for linear strels
    node_module_register({
        "Strel",
        "Select elements for a morphological operation",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<std::pair<std::vector<uint8_t>, std::vector<int>>, npk::attribute>;
            auto val = std::make_pair(std::vector<uint8_t>(9, 1), std::vector{3, 3});
            auto attr = attribute("Value");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_val, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(&attr, 1);
            };

            auto render = [](node* self) -> void {
                auto& [val, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [data, shape] = val;
                auto id = std::format("node_table_{}", self->uid());
                auto shape_sum = std::accumulate(shape.begin(), shape.end(), 0);

                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImNodes::BeginOutputAttribute(attr.uid());
                ImGui::BeginGroup();
                ImGui::PushItemWidth(24*3);
                for (auto i = 0; i < shape.size(); i++) {
                    ImGui::PushID(i);
                    ImGui::SliderInt(std::format("D{}", i).c_str(), &shape[i], 3, 48);
                    ImGui::PopID();
                }
                ImGui::PopItemWidth();
                ImGui::EndGroup();
                if (ImGui::BeginTable(id.c_str(), shape[1], 0, {24.f*shape[1], 0})) {
                    for (auto i = 0; i < data.size(); i++) {
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(24);
                        ImGui::PushID(i);
                        ImGui::Checkbox("", reinterpret_cast<bool*>(&data[i]));
                        ImGui::PopID();
                        ImGui::PopItemWidth();
                    }
                    ImGui::EndTable();
                }
                ImNodes::EndOutputAttribute();
                ImNodes::EndNode();

                if (std::accumulate(shape.begin(), shape.end(), 0) != shape_sum) {
                    data.resize(std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>()));
                    std::fill(data.begin(), data.end(), 1);
                }
            };

            auto evaluate = [](node* self) -> void {
                auto& [val, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                attr.set(&val);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(val), std::move(attr)),
                attributes,
                render,
                evaluate
            );
        }
    });

    current_category = std::string_view("Processors");

    node_module_register({
        "Adjust brightness",
        "Adjust brightness level of a given image by given value",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Factor");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted("Image");
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted("Factor");
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted("Image");
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<float*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                img.unload();
                img.clone(inimg);
                adjust_brightness_2d(&img, *inval);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Adjust contrast",
        "Adjust contrast level of a given image by given value",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Factor");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted("Image");
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted("Factor");
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted("Image");
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<float*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                img.unload();
                img.clone(inimg);
                adjust_contrast_2d(&img, *inval);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Adjust color",
        "Adjust color level of a given image by given value",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inclr_attr = attribute("Color");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inclr_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inclr_attr.uid());
                ImGui::TextUnformatted(inclr_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inclr_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inclr = reinterpret_cast<color*>(inclr_attr.get());
                if (inclr == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                img.unload();
                img.clone(inimg);
                adjust_color_2d(&img, *inclr);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inclr_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Grayscaling",
        "Convert a given image into grayscale",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 2>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                if (inimg == nullptr || inimg->data == nullptr) return;

                img.unload();
                img.clone(inimg);
                convert_gray_2d(&img);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Invertion",
        "Invert color levels of a given image",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 2>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                if (inimg == nullptr || inimg->data == nullptr) return;

                img.unload();
                img.clone(inimg);
                invert_color_2d(&img);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Thresholding",
        "Changes given image pixel channels to extreme levels based on a given threshold factor",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Factor");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted(inval_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<float*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                img.unload();
                img.clone(inimg);
                thresholding_2d(&img, *inval);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Binarization",
        "Converts given image to binary based on a given threshold factor",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Factor");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted(inval_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<float*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                img.unload();
                img.clone(inimg);
                binarization_2d(&img, *inval);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Minimum",
        "Outputs the minimum value between two images",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto in1img_attr = attribute("Image");
            auto in2img_attr = attribute("Image");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [in1img_attr, in2img_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(in1img_attr.uid());
                ImGui::TextUnformatted(in1img_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(in2img_attr.uid());
                ImGui::TextUnformatted(in2img_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto in1img = reinterpret_cast<image*>(inimg_attr.get());
                auto in2img = reinterpret_cast<image*>(inval_attr.get());
                if (in1img == nullptr || in1img->data == nullptr || in2img == nullptr || in2img->data == nullptr) return;

                img.unload();
                img.clone(in2img);
                min_2d(in1img, &img);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(in1img_attr), std::move(in2img_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Maximum",
        "Outputs the maximum value between two images",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto in1img_attr = attribute("Image");
            auto in2img_attr = attribute("Image");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [in1img_attr, in2img_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(in1img_attr.uid());
                ImGui::TextUnformatted(in1img_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(in2img_attr.uid());
                ImGui::TextUnformatted(in2img_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto in1img = reinterpret_cast<image*>(inimg_attr.get());
                auto in2img = reinterpret_cast<image*>(inval_attr.get());
                if (in1img == nullptr || in1img->data == nullptr || in2img == nullptr || in2img->data == nullptr) return;

                img.unload();
                img.clone(in2img);
                max_2d(in1img, &img);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(in1img_attr), std::move(in2img_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Sum",
        "Outputs the sum between two images",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto in1img_attr = attribute("Image");
            auto in2img_attr = attribute("Image");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [in1img_attr, in2img_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(in1img_attr.uid());
                ImGui::TextUnformatted(in1img_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(in2img_attr.uid());
                ImGui::TextUnformatted(in2img_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto in1img = reinterpret_cast<image*>(inimg_attr.get());
                auto in2img = reinterpret_cast<image*>(inval_attr.get());
                if (in1img == nullptr || in1img->data == nullptr || in2img == nullptr || in2img->data == nullptr) return;

                img.unload();
                img.clone(in2img);
                sum_2d(in1img, &img);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(in1img_attr), std::move(in2img_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Difference",
        "Outputs the difference between two images",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto in1img_attr = attribute("Image");
            auto in2img_attr = attribute("Image");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [in1img_attr, in2img_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(in1img_attr.uid());
                ImGui::TextUnformatted(in1img_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(in2img_attr.uid());
                ImGui::TextUnformatted(in2img_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto in1img = reinterpret_cast<image*>(inimg_attr.get());
                auto in2img = reinterpret_cast<image*>(inval_attr.get());
                if (in1img == nullptr || in1img->data == nullptr || in2img == nullptr || in2img->data == nullptr) return;

                img.unload();
                img.clone(in2img);
                sub_2d(in1img, &img);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(in1img_attr), std::move(in2img_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Product",
        "Outputs the product between two images",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto in1img_attr = attribute("Image");
            auto in2img_attr = attribute("Image");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [in1img_attr, in2img_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(in1img_attr.uid());
                ImGui::TextUnformatted(in1img_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(in2img_attr.uid());
                ImGui::TextUnformatted(in2img_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto in1img = reinterpret_cast<image*>(inimg_attr.get());
                auto in2img = reinterpret_cast<image*>(inval_attr.get());
                if (in1img == nullptr || in1img->data == nullptr || in2img == nullptr || in2img->data == nullptr) return;

                img.unload();
                img.clone(in2img);
                mul_2d(in1img, &img);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(in1img_attr), std::move(in2img_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Gaussian blur",
        "Smooth image based on a space deviation",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Space Deviation");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted(inval_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<float*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                img.unload();
                img.similar(inimg);
                gaussian_2d(inimg, &img, *inval);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Bilateral blur",
        "Smooth image and preserve edges based on gaussian deviation for space and color",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 4>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto in1val_attr = attribute("Space Deviation");
            auto in2val_attr = attribute("Color Deviation");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, in1val_attr, in2val_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(in1val_attr.uid());
                ImGui::TextUnformatted(in1val_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(in2val_attr.uid());
                ImGui::TextUnformatted(in2val_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, in1val_attr, in2val_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto in1val = reinterpret_cast<float*>(in1val_attr.get());
                auto in2val = reinterpret_cast<float*>(in2val_attr.get());
                if (in1val == nullptr || in2val == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                img.unload();
                img.similar(inimg);
                bilateral_2d(inimg, &img, *in1val, *in2val);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(in1val_attr), std::move(in2val_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Mean blur",
        "Smooth image by taking the mean of the neighboring pixels",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Radius");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted(inval_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<float*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                img.unload();
                img.similar(inimg);
                mean_2d(inimg, &img, *inval);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Convolution",
        "Apply a filter to the pixel based on neighboring pixels",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Filter");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted(inval_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<std::pair<std::vector<float>, std::vector<int>>*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                auto& [data, shape] = *inval;

                img.unload();
                img.similar(inimg);
                convolve_2d(inimg, &img, data, shape);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Erosion",
        "Apply a erosion to the pixel based on neighboring pixels",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Strel");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted(inval_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<std::pair<std::vector<uint8_t>, std::vector<int>>*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                auto& [data, shape] = *inval;

                img.unload();
                img.similar(inimg);
                erode_2d(inimg, &img, data, shape);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Dilation",
        "Apply a dilation to the pixel based on neighboring pixels",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Strel");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted(inval_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<std::pair<std::vector<uint8_t>, std::vector<int>>*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                auto& [data, shape] = *inval;

                img.unload();
                img.similar(inimg);
                dilate_2d(inimg, &img, data, shape);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Opening",
        "Apply a opening to the pixel based on neighboring pixels",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Strel");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted(inval_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<std::pair<std::vector<uint8_t>, std::vector<int>>*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                auto& [data, shape] = *inval;

                img.unload();
                img.similar(inimg);
                open_2d(inimg, &img, data, shape);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    node_module_register({
        "Closing",
        "Apply a closing to the pixel based on neighboring pixels",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Strel");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted(inval_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<std::pair<std::vector<uint8_t>, std::vector<int>>*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                auto& [data, shape] = *inval;

                img.unload();
                img.similar(inimg);
                close_2d(inimg, &img, data, shape);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    // INFO: Not a good description
    node_module_register({
        "White Tophat",
        "Apply a white tophat to the pixel based on neighboring pixels",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Strel");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted(inval_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<std::pair<std::vector<uint8_t>, std::vector<int>>*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                auto& [data, shape] = *inval;

                img.unload();
                img.similar(inimg);
                white_tophat_2d(inimg, &img, data, shape);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    // INFO: Not a good description
    node_module_register({
        "Black Tophat",
        "Apply a black tophat to the pixel based on neighboring pixels",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<npk::image, std::array<npk::attribute, 3>>;
            auto img = image();
            auto inimg_attr = attribute("Image");
            auto inval_attr = attribute("Strel");
            auto outimg_attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(attrs);
            };

            auto render = [](node* self) -> void {
                auto& [_img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();

                ImNodes::BeginInputAttribute(inimg_attr.uid());
                ImGui::TextUnformatted(inimg_attr.name().data());
                ImNodes::EndInputAttribute();

                ImNodes::BeginInputAttribute(inval_attr.uid());
                ImGui::TextUnformatted(inval_attr.name().data());
                ImNodes::EndInputAttribute();

                ImGui::SameLine();

                ImNodes::BeginOutputAttribute(outimg_attr.uid());
                ImGui::TextUnformatted(outimg_attr.name().data());
                ImNodes::EndOutputAttribute();

                ImGui::EndGroup();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [img, attrs] = std::any_cast<intrinsic&>(self->intrinsic());
                auto& [inimg_attr, inval_attr, outimg_attr] = attrs;
                auto inimg = reinterpret_cast<image*>(inimg_attr.get());
                auto inval = reinterpret_cast<std::pair<std::vector<uint8_t>, std::vector<int>>*>(inval_attr.get());
                if (inval == nullptr || inimg == nullptr || inimg->data == nullptr) return;

                auto& [data, shape] = *inval;

                img.unload();
                img.similar(inimg);
                black_tophat_2d(inimg, &img, data, shape);
                outimg_attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(img), {std::move(inimg_attr), std::move(inval_attr), std::move(outimg_attr)}),
                attributes,
                render,
                evaluate
            );
        }
    });

    current_category = std::string_view("Visualizers");

    node_module_register({
        "Image Visualizer",
        "Visualize a given image",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = npk::attribute;
            auto attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& attr = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(&attr, 1);
            };

            auto render = [](node* self) -> void {
                auto& attr = std::any_cast<intrinsic&>(self->intrinsic());
                auto img = reinterpret_cast<image*>(attr.get());
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                constexpr auto base = 256;
                auto width = base;
                auto height = base;
                if (img) {
                    if (img->width > img->height) width = img->width * base / img->height;
                    if (img->height > img->width) height = img->height * base / img->width;
                }
                ImGui::Image(img ? img->texture : nullptr, ImVec2(width, height));

                ImNodes::BeginInputAttribute(attr.uid());
                ImGui::TextUnformatted(attr.name().data());
                ImNodes::EndInputAttribute();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& attr = std::any_cast<intrinsic&>(self->intrinsic());
                auto img = reinterpret_cast<image*>(attr.get());
                if (img != nullptr && img->data != nullptr && img->texture == nullptr) img->upload();
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                attr,
                attributes,
                render,
                evaluate
            );
        }
    });
}

} // namespace npk
