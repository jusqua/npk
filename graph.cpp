#include <print>
#include <ranges>

#include <imgui.h>
#include <imnodes.h>

#include "node.hpp"
#include "link.hpp"
#include "graph.hpp"

namespace npk {

auto graph::attach_node(node&& n) -> void {
    ImNodes::SetNodeScreenSpacePos(n.uid(), ImGui::GetMousePos());

    m_links.insert({n.uid(), {}});
    m_marked.insert({n.uid(), false});
    m_nodes.insert({n.uid(), std::move(n)});
}

auto graph::detach_node(int uid) -> void {
    m_links.erase(uid);
    m_marked.erase(uid);
    m_nodes.erase(uid);

    for (auto& [_, ls] : m_links) {
        for (auto it = ls.begin(); it != ls.end(); it++) {
            if (!it->with_node(uid)) continue;

            ls.erase(it);
            return;
        }
    }
}

auto graph::find_node_from_attribute(int uid) -> node* {
    for (auto& [i, n] : m_nodes) {
        for (auto& a : n.attributes()) {
            if (a.uid() == uid) {
                return &n;
            };
        }
    }
    return nullptr;
}

auto graph::create_link(int from, int to) -> void {
    auto from_node = find_node_from_attribute(from);
    auto to_node = find_node_from_attribute(to);

    if (from_node == nullptr || to_node == nullptr) {
        std::println("NPK Error: Attributes not found to create a link");
        return;
    }

    m_links[from_node->uid()].push_back(link(from_node->uid(), from, to_node->uid(), to));
}

auto graph::delete_link(int uid) -> void {
    for (auto& [_, ls] : m_links) {
        for (auto it = ls.begin(); it != ls.end(); it++) {
            if (it->uid() != uid) continue;

            ls.erase(it);
            return;
        }
    }
}

auto graph::visit(std::stack<int>& stack, node& n) -> void {
    m_marked[n.uid()] = true;

    for (auto& l : m_links[n.uid()]) {
        auto& ng = m_nodes.at(l.to_node());
        if (m_marked[ng.uid()]) continue;

        visit(stack, ng);
    }

    stack.push(n.uid());
}

auto graph::traverse(std::stack<int>& stack) -> void {
    for (auto& [_, m] : m_marked) {
        m = false;
    }
    for (auto& [_, n] : m_nodes) {
        for (auto& a : n.attributes()) {
            a.set(nullptr);
        }
    }

    for (auto& [_, n] : m_nodes) {
        if (m_marked[n.uid()]) continue;

        visit(stack, n);
    }
}

// TODO: Gerenate different stack for every tree in a forest
auto graph::evaluate(void) -> void {
    std::stack<int> stack;

    traverse(stack);

    if (stack.empty()) return;

    std::print("NPK Info: Evaluation Order = ");
    while (!stack.empty()) {
        auto& n = m_nodes.at(stack.top());
        stack.pop();

        std::print("{}", n.name());
        if (!stack.empty()) std::print(", ");
        n.evaluate();

        // TODO: Make a better approach to find and set attributes
        for (auto& l : m_links[n.uid()]) {
            auto& ng = m_nodes.at(l.to_node());
            auto [from, to] = l.attributes();

            for (auto& out : n.attributes()) {
                if (out.uid() != from) continue;

                for (auto& in : ng.attributes()) {
                    if (in.uid() != to) continue;

                    in.set(out.get());
                }
            }
        }
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
        for (auto& category : std::views::keys(node_module_map())) {
            if (ImGui::BeginMenu(category.data())) {
                for (auto& module : std::views::all(node_module_map().at(category))) {
                    if (ImGui::MenuItem(module.name().data()))
                        attach_node(module.instantiate());
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {6, 6});
                        ImGui::SetTooltip("%s", module.description().data());
                        ImGui::PopStyleVar();
                    }
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    for (auto& [_, n] : m_nodes) n.render();
    for (auto& [_, ls] : m_links) for (auto& l : ls) l.render();
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

