#include <filesystem>
#include <map>

#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>
#include <nfd.hpp>

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
auto node_module_init(void) -> void {
    auto current_category = std::string_view("Selectors");

    node_module_register({
        "Image",
        "Select an image from system",
        current_category,
        [](node_module* module) -> node {
            using intrinsic = std::tuple<std::filesystem::path, npk::image, npk::attribute>;
            auto filepath = std::filesystem::path();
            auto img = image();
            auto attr = attribute("Image");

            auto attributes = [](node* self) -> std::span<attribute> {
                auto& [_filepath, _img, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                return std::span(&attr, 1);
            };

            auto render = [](node* self) -> void {
                auto& [filepath, img, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                ImNodes::BeginNode(self->uid());

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(self->name().data());
                ImNodes::EndNodeTitleBar();

                ImGui::BeginGroup();
                if (ImGui::Button("^", {16, 16})) {
                    auto path = NFD::UniquePath();
                    auto filter = std::vector<nfdfilteritem_t>{{"PNG", "png"}};
                    auto result = NFD::OpenDialog(path, filter.data(), filter.size());
                    if (result == NFD_OKAY) {
                        filepath = path.get();
                        img.unload();
                        img.load(path.get());
                        img.upload();
                    }
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {6, 6});
                    ImGui::SetTooltip("Open image file");
                    ImGui::PopStyleVar();
                }

                ImGui::SameLine();

                ImGui::TextDisabled("%s", filepath.filename().c_str());
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {6, 6});
                    ImGui::SetTooltip("%s", filepath.c_str());
                    ImGui::PopStyleVar();
                }
                ImGui::EndGroup();

                ImNodes::BeginOutputAttribute(attr.uid());
                ImGui::Image(img.texture_id, ImVec2(256, 256));
                ImNodes::EndOutputAttribute();

                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& [_filepath, img, attr] = std::any_cast<intrinsic&>(self->intrinsic());
                attr.set(&img);
            };

            return node(
                module->name(),
                module->description(),
                module->category(),
                intrinsic(std::move(filepath), std::move(img), std::move(attr)),
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

                ImGui::Image(img != nullptr ? img->texture_id : 0, ImVec2(256, 256));

                ImNodes::BeginInputAttribute(attr.uid());
                ImGui::TextUnformatted(attr.name().data());
                ImNodes::EndInputAttribute();
                ImNodes::EndNode();
            };

            auto evaluate = [](node* self) -> void {
                auto& attr = std::any_cast<intrinsic&>(self->intrinsic());
                auto img = reinterpret_cast<image*>(attr.get());
                if (img != nullptr && img->data != nullptr && img->texture_id == 0) img->upload();
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
