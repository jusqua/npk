#pragma once

#include <any>
#include <functional>
#include <list>
#include <span>
#include <map>

#include "attribute.hpp"

namespace npk {

class node {
private:
    int m_uid;
    std::string_view m_name;
    std::string_view m_description;
    std::string_view m_category;

    std::any m_intrinsic;
    std::function<std::span<attribute>(node*)> m_func_attributes;
    std::function<void(node*)> m_func_render;
    std::function<void(node*)> m_func_evaluate;

public:
    node(std::string_view name, std::string_view description, std::string_view category, std::any&& intrinsic,
        std::function<std::span<attribute>(node*)>&& func_attibutes, std::function<void(node*)>&& func_render, std::function<void(node*)>&& func_evaluate);

    auto uid(void) -> int;
    auto name(void) -> std::string_view;
    auto description(void) -> std::string_view;
    auto category(void) -> std::string_view;

    auto intrinsic(void) -> std::any&;
    auto attributes(void) -> std::span<attribute>;
    auto render(void) -> void;
    auto evaluate(void) -> void;
};

class node_module {
private:
    std::string_view m_name;
    std::string_view m_description;
    std::string_view m_category;
    std::function<node(node_module*)> m_func_instantiate;

public:
    node_module(std::string_view name, std::string_view description, std::string_view category, std::function<node(node_module*)>&& func_instantiate);

    auto name(void) -> std::string_view;
    auto description(void) -> std::string_view;
    auto category(void) -> std::string_view;
    auto instantiate() -> node;
};

auto node_module_register(node_module&& module) -> void;
auto node_module_map(void) -> std::map<std::string, std::list<npk::node_module>>&;
auto node_module_init(void) -> void;

} // namespace npk

