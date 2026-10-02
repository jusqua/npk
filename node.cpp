#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>
#include <nfd.hpp>

#include "utils.hpp"
#include "processing.hpp"
#include "node.hpp"

namespace npk {

node::node(node_kind kind) : uid(get_uid()), kind(kind) {
    switch (kind) {
    case node_kind::image:
        title = "Image Picker";
        data.img = std::make_shared<image>();
        outattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        break;
    case node_kind::color:
        title = "Color Picker";
        data.clr = color(1, 1, 1, 1);
        outattributes.push_back(std::make_shared<attribute>(attribute_kind::color));
        break;
    case node_kind::value:
        title = "Value Picker";
        data.value = 0;
        outattributes.push_back(std::make_shared<attribute>(attribute_kind::value));
        break;
    case node_kind::adjust_intensity:
        title = "Adjust Intensity";
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::value));
        outattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        break;
    case node_kind::adjust_contrast:
        title = "Adjust Contrast";
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::value));
        outattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        break;
    case node_kind::adjust_color:
        title = "Adjust Color";
        data.img = nullptr;
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::color));
        outattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        break;
    case node_kind::convert_gray:
        title = "Convert to Grayscale";
        data.img = nullptr;
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        outattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        break;
    case node_kind::invert_color:
        title = "Invert Color";
        data.img = nullptr;
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        outattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        break;
    case node_kind::thresholding:
        title = "Thresholding";
        data.img = nullptr;
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::value));
        outattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        break;
    case node_kind::binarization:
        title = "Binarization";
        data.img = nullptr;
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::value));
        outattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        break;
    case node_kind::display_image:
        title = "Display Image";
        data.img = nullptr;
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        break;
    };
}

auto node::render(void) -> void {
    ImNodes::BeginNode(uid);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(title.data());
    ImNodes::EndNodeTitleBar();
    switch (kind) {
    case node_kind::image:
        ImGui::BeginGroup();
        if (ImGui::Button("^", {16, 16})) {
            auto path = NFD::UniquePath();
            auto filter = std::vector<nfdfilteritem_t>{{"PNG", "png"}};
            auto result = NFD::OpenDialog(path, filter.data(), filter.size());
            if (result == NFD_OKAY) {
                data.path = path.get();
                data.img->unload();
                data.img->load(path.get());
                data.img->upload();
            }
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {6, 6});
            ImGui::SetTooltip("Open image file");
            ImGui::PopStyleVar();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", data.path.filename().c_str());
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {6, 6});
            ImGui::SetTooltip("%s", data.path.c_str());
            ImGui::PopStyleVar();
        }
        ImGui::EndGroup();

        ImNodes::BeginOutputAttribute(outattributes[0]->uid);
        ImGui::Image(data.img->texture_id, ImVec2(256, 256));
        ImNodes::EndOutputAttribute();
        break;
    case node_kind::color:

        ImNodes::BeginOutputAttribute(outattributes[0]->uid);
        ImGui::PushItemWidth(256);
        ImGui::ColorPicker4("", reinterpret_cast<float*>(&data.clr), ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview);
        ImGui::PopItemWidth();
        ImNodes::EndOutputAttribute();

        break;
    case node_kind::value:
        // XXX: This "value" node is too repetitive
        ImNodes::BeginOutputAttribute(outattributes[0]->uid);
        ImGui::PushItemWidth(96);
        ImGui::InputFloat("", &data.value, 1.f, 1.f);
        ImGui::PopItemWidth();
        ImNodes::EndOutputAttribute();

        break;
    case node_kind::adjust_intensity:
    case node_kind::adjust_contrast:
        ImGui::BeginGroup();

        ImNodes::BeginInputAttribute(inattributes[0]->uid);
        ImGui::TextUnformatted("Image");
        ImNodes::EndInputAttribute();

        ImNodes::BeginInputAttribute(inattributes[1]->uid);
        ImGui::TextUnformatted("Factor");
        ImNodes::EndInputAttribute();

        ImGui::SameLine();

        ImNodes::BeginOutputAttribute(outattributes[0]->uid);
        ImGui::TextUnformatted("Image");
        ImNodes::EndOutputAttribute();

        ImGui::EndGroup();
        break;
    case node_kind::adjust_color:
        ImGui::BeginGroup();

        ImNodes::BeginInputAttribute(inattributes[0]->uid);
        ImGui::TextUnformatted("Image");
        ImNodes::EndInputAttribute();

        ImNodes::BeginInputAttribute(inattributes[1]->uid);
        ImGui::TextUnformatted("Color");
        ImNodes::EndInputAttribute();

        ImGui::SameLine();

        ImNodes::BeginOutputAttribute(outattributes[0]->uid);
        ImGui::TextUnformatted("Image");
        ImNodes::EndOutputAttribute();

        ImGui::EndGroup();
        break;
    case node_kind::convert_gray:
    case node_kind::invert_color:
        ImGui::BeginGroup();

        ImNodes::BeginInputAttribute(inattributes[0]->uid);
        ImGui::TextUnformatted("Image");
        ImNodes::EndInputAttribute();

        ImGui::SameLine();

        ImNodes::BeginOutputAttribute(outattributes[0]->uid);
        ImGui::TextUnformatted("Image");
        ImNodes::EndOutputAttribute();

        ImGui::EndGroup();
        break;
    case node_kind::thresholding:
    case node_kind::binarization:
        ImGui::BeginGroup();

        ImNodes::BeginInputAttribute(inattributes[0]->uid);
        ImGui::TextUnformatted("Image");
        ImNodes::EndInputAttribute();

        ImNodes::BeginInputAttribute(inattributes[1]->uid);
        ImGui::TextUnformatted("Threshold");
        ImNodes::EndInputAttribute();

        ImGui::SameLine();

        ImNodes::BeginOutputAttribute(outattributes[0]->uid);
        ImGui::TextUnformatted("Image");
        ImNodes::EndOutputAttribute();

        ImGui::EndGroup();
        break;
    case node_kind::display_image:
        ImGui::Image(data.img != nullptr ? data.img->texture_id : 0, ImVec2(256, 256));

        ImNodes::BeginInputAttribute(inattributes[0]->uid);
        ImGui::TextUnformatted("Image");
        ImNodes::EndInputAttribute();
        break;
    }
    ImNodes::EndNode();
}

auto node::evaluate(std::stack<node_data*>& stack) -> void {
    switch (kind) {
    case node_kind::image:
    case node_kind::color:
    case node_kind::value:
        stack.push(&data);
        break;
    case node_kind::adjust_intensity: {
        if (stack.size() != 2) return;

        auto value = stack.top()->value;
        stack.pop();

        data.img = std::make_shared<image>();
        data.img->clone(stack.top()->img.get());
        stack.pop();
        auto& img = data.img;

        adjust_intensity_2d(img.get(), value);

        stack.push(&data);

        break;
    }
    case node_kind::adjust_contrast: {
        if (stack.size() != 2) return;

        auto value = stack.top()->value;
        stack.pop();

        data.img = std::make_shared<image>();
        data.img->clone(stack.top()->img.get());
        stack.pop();
        auto& img = data.img;

        adjust_contrast_2d(img.get(), value);

        stack.push(&data);

        break;
    }
    case node_kind::adjust_color: {
        if (stack.size() != 2) return;

        auto clr = stack.top()->clr;
        stack.pop();

        data.img = std::make_shared<image>();
        data.img->clone(stack.top()->img.get());
        stack.pop();
        auto& img = data.img;

        adjust_color_2d(img.get(), clr);

        stack.push(&data);

        break;
    }
    case node_kind::convert_gray: {
        if (stack.size() != 1) return;

        data.img = std::make_shared<image>();
        data.img->clone(stack.top()->img.get());
        stack.pop();
        auto& img = data.img;

        convert_gray_2d(img.get());

        stack.push(&data);

        break;
    }
    case node_kind::invert_color: {
        if (stack.size() != 1) return;

        data.img = std::make_shared<image>();
        data.img->clone(stack.top()->img.get());
        stack.pop();
        auto& img = data.img;

        invert_color_2d(img.get());

        stack.push(&data);

        break;
    }
    case node_kind::thresholding: {
        if (stack.size() != 2) return;

        auto threshold = stack.top()->value;
        stack.pop();

        data.img = std::make_shared<image>();
        data.img->clone(stack.top()->img.get());
        stack.pop();
        auto& img = data.img;

        thresholding_2d(img.get(), threshold);

        stack.push(&data);

        break;
    }
    case node_kind::binarization: {
        if (stack.size() != 2) return;

        auto threshold = stack.top()->value;
        stack.pop();

        data.img = std::make_shared<image>();
        data.img->clone(stack.top()->img.get());
        stack.pop();
        auto& img = data.img;

        binarization_2d(img.get(), threshold);

        stack.push(&data);

        break;
    }
    case node_kind::display_image:
        if (stack.size() != 1) return;

        data.img = stack.top()->img;
        stack.pop();
        data.img->upload();
        break;
    };
}

} // namespace npk
