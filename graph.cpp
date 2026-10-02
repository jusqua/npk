#include <print>

#include <imgui.h>
#include <imnodes.h>

#include "graph.hpp"

namespace npk {

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

// TODO: Gerenate different stack for every tree in a forest
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
        if (ImGui::MenuItem("Image Picker"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::image));
        if (ImGui::MenuItem("Color Picker"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::color));
        if (ImGui::MenuItem("Value Picker"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::value));
        ImGui::Separator();
        if (ImGui::MenuItem("Adjust Intensity"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::adjust_intensity));
        if (ImGui::MenuItem("Adjust Constrast"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::adjust_contrast));
        if (ImGui::MenuItem("Adjust Color"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::adjust_color));
        if (ImGui::MenuItem("Invert Color"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::invert_color));
        if (ImGui::MenuItem("Convert to Grayscale"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::convert_gray));
        if (ImGui::MenuItem("Thresholding"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::thresholding));
        if (ImGui::MenuItem("Binarization"))
            attach_node(std::make_shared<npk::node>(npk::node_kind::binarization));
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

