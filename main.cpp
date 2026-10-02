#include "application.hpp"
#include "image.hpp"
#include "color.hpp"
#include "processing.hpp"

#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <imnodes.h>
#include <nfd.hpp>
#include <GLFW/glfw3.h>

#include <memory>
#include <stack>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <print>

namespace npk {

static int uid_count = 0;

static int get_uid(void) {
    return ++uid_count;
}

enum struct attribute_kind {
    image,
    color,
};

enum struct node_kind {
    image,
    color,
    adjust_color,
    display_image,
};

struct attribute {
    int uid;
    std::string_view description;
    attribute_kind kind;

    attribute(attribute_kind kind);
};

attribute::attribute(attribute_kind kind)
    : uid(get_uid()), kind(kind) {}

struct link {
    int uid;
    int from_node;
    int from_attribute;
    int to_node;
    int to_attribute;

    link(int from_node, int from_attribute, int to_node, int to_attribute);
    auto render(void) -> void;
};

link::link(int from_node, int from_attribute, int to_node, int to_attribute)
    : uid(get_uid()), from_node(from_node), from_attribute(from_attribute), to_node(to_node), to_attribute(to_attribute) {}

auto link::render(void) -> void {
    ImNodes::Link(uid, from_attribute, to_attribute);
}

enum struct node_mark {
    tree,
    back,
    forward
};

struct node_data {
    std::filesystem::path path;
    std::shared_ptr<image> img;
    color clr;
};

struct node {
    int uid;
    std::string_view title;
    node_kind kind;
    node_mark mark;
    node_data data;
    std::vector<std::shared_ptr<attribute>> inattributes;
    std::vector<std::shared_ptr<attribute>> outattributes;

    node(node_kind kind);

    auto render(void) -> void;
    auto evaluate(std::stack<node_data*>& stack) -> void;
};

node::node(node_kind kind) : uid(get_uid()), kind(kind) {
    switch (kind) {
    case node_kind::image:
        title = "Image";
        data.img = std::make_shared<image>();
        outattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        break;
    case node_kind::color:
        title = "Color";
        data.clr = color(1, 1, 1, 1);
        outattributes.push_back(std::make_shared<attribute>(attribute_kind::color));
        break;
    case node_kind::adjust_color:
        title = "Adjust Color";
        data.img = nullptr;
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::image));
        inattributes.push_back(std::make_shared<attribute>(attribute_kind::color));
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
    switch (kind) {
    case node_kind::image:
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Image Picker");
        ImNodes::EndNodeTitleBar();

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

        ImGui::Image(data.img->texture_id, ImVec2(256, 256));

        ImNodes::BeginOutputAttribute(outattributes[0]->uid);
        ImGui::TextAligned(1.f, 256, "Image");
        ImNodes::EndOutputAttribute();
        break;
    case node_kind::color:
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Color Picker");
        ImNodes::EndNodeTitleBar();

        ImGui::PushItemWidth(256);
        ImGui::ColorPicker4("", reinterpret_cast<float*>(&data.clr), ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview);
        ImGui::PopItemWidth();

        ImNodes::BeginOutputAttribute(outattributes[0]->uid);
        ImGui::TextAligned(1.f, 256, "Color");
        ImNodes::EndOutputAttribute();

        break;
    case node_kind::adjust_color:
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Adjust Color");
        ImNodes::EndNodeTitleBar();

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
    case node_kind::display_image:
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Display Image");
        ImNodes::EndNodeTitleBar();

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
        stack.push(&data);
        break;
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
    case node_kind::display_image:
        if (stack.size() != 1) return;

        data.img = stack.top()->img;
        stack.pop();
        data.img->upload();
        break;
    };
}

struct graph {
    std::unordered_map<int, std::vector<std::shared_ptr<node>>> neighbors;
    std::unordered_map<int, std::shared_ptr<node>> nodes;
    std::unordered_map<int, std::shared_ptr<link>> links;

    auto attach_node(std::shared_ptr<node>&& n);
    auto detach_node(int uid);
    auto create_link(int from, int to);
    auto delete_link(int uid);

    auto visit(std::stack<int>& stack, std::shared_ptr<node>& n) -> void;
    auto traverse(std::stack<int>& stack) -> void;
    auto evaluate(void) -> void;

    auto render(void) -> void;
};

auto graph::attach_node(std::shared_ptr<node>&& n) {
    nodes[n->uid] = n;
    neighbors[n->uid] = {};
    ImNodes::SetNodeScreenSpacePos(n->uid, ImGui::GetMousePos());
}

auto graph::detach_node(int uid) {
    nodes.erase(uid);
    neighbors.erase(uid);

    for (auto it = links.begin(); it != links.end();) {
        auto& l = it->second;
        if (l->from_node != uid && l->to_node != uid) {
            it++;
            continue;
        }

        it = links.erase(it);
    }
}

auto graph::create_link(int from, int to) {
    std::shared_ptr<node> from_node;
    std::shared_ptr<node> to_node;
    for (auto& [i, n] : nodes) {
        for (auto& a : n->outattributes) {
            if (a->uid != from) continue;
            from_node = n;
        }
    }
    for (auto& [i, n] : nodes) {
        for (auto& a : n->inattributes) {
            if (a->uid != to) continue;
            to_node = n;
        }
    }

    auto new_link = std::make_shared<link>(from_node->uid, from, to_node->uid, to);
    links[new_link->uid] = std::move(new_link);
    neighbors[from_node->uid].push_back(to_node);
}

auto graph::delete_link(int uid) {
    links.erase(uid);
}

auto graph::visit(std::stack<int>& stack, std::shared_ptr<node>& n) -> void {
    n->mark = node_mark::back;

    for (auto& ng : neighbors[n->uid]) {
        if (ng->mark != node_mark::tree) continue;

        visit(stack, ng);
    }

    n->mark = node_mark::forward;
    stack.push(n->uid);
}

auto graph::traverse(std::stack<int>& stack) -> void {
    for (auto& [_, n] : nodes) {
        n->mark = node_mark::tree;
    }

    for (auto& [_, n] : nodes) {
        if (n->mark != node_mark::tree) continue;

        visit(stack, n);
    }
}

auto graph::evaluate(void) -> void {
    std::stack<int> trail;
    std::stack<node_data*> stack;

    traverse(trail);

    if (!trail.empty())
        std::print("NPK Info: Evaluation Order = ");
    while (!trail.empty()) {
        auto& n = nodes[trail.top()];
        trail.pop();

        std::print("{}", n->title);
        if (!trail.empty()) std::print(", ", n->title);
        n->evaluate(stack);
    }
    std::println();
}

auto graph::render(void) -> void {
    constexpr auto toolbar_size = 24;
    auto viewport = ImGui::GetMainViewport();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar
                                  | ImGuiWindowFlags_NoResize
                                  | ImGuiWindowFlags_NoMove
                                  | ImGuiWindowFlags_NoCollapse
                                  | ImGuiWindowFlags_NoBringToFrontOnFocus
                                  | ImGuiWindowFlags_NoSavedSettings
                                  | ImGuiWindowFlags_NoNavFocus;
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize({viewport->WorkSize.x, viewport->WorkSize.y - toolbar_size});
    ImGui::Begin("Main", nullptr, window_flags);

    ImNodes::BeginNodeEditor();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {10, 10});
    if (ImGui::BeginPopupContextWindow("Add Node Menu")) {
        if (ImGui::MenuItem("Image"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::image));
        if (ImGui::MenuItem("Color"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::color));
        ImGui::Separator();
        if (ImGui::MenuItem("Adjust Color"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::adjust_color));
        ImGui::Separator();
        if (ImGui::MenuItem("Display Image"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::display_image));
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    for (auto& [_, n] : nodes) n->render();
    for (auto& [_, l] : links) l->render();
    ImNodes::EndNodeEditor();

    int from, to, uid;
    if (ImNodes::IsLinkCreated(&from, &to)) create_link(from, to);
    if (ImNodes::IsLinkDestroyed(&uid)) delete_link(uid);

    if (ImGui::IsKeyReleased(ImGuiKey_Delete) && ImNodes::NumSelectedNodes() > 0) {
        auto to_delete = std::vector<int>(ImNodes::NumSelectedNodes());
        ImNodes::GetSelectedNodes(to_delete.data());
        for (auto& uid : to_delete) {
            detach_node(uid);
        }
        ImNodes::ClearNodeSelection();
    }

    ImGui::End();

    ImGui::SetNextWindowPos({0, viewport->WorkSize.y - toolbar_size});
    ImGui::SetNextWindowSize({viewport->WorkSize.x, toolbar_size});
    ImGui::Begin("Toolbar", nullptr, window_flags);
    if (ImGui::Button("Evaluate", {0, toolbar_size})) evaluate();
    ImGui::End();

    ImGui::PopStyleVar(3);
}

} // namespace npk

int main(void) {
    auto app = npk::application();

    auto graph = npk::graph();

    app.render([&] {
        graph.render();
    });
}
