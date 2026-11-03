#include "application.hpp"
#include "image.hpp"
#include "node.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imnodes.h>
#include <nfd.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <print>

static GLFWwindow* window = nullptr;

static auto glfw_error_callback(int error, const char* description) -> void {
    std::println(std::cerr, "GLFW Error {}: {}", error, description);
}

namespace npk {

application::application() {
    NFD::Init();
    if (window != nullptr) {
        std::println(std::cerr, "NPK Error: Application already instantiated");
        abort();
    }

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::println(std::cerr, "NPK Error: Could not initialize GLFW");
        abort();
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    auto primary_monitor = glfwGetPrimaryMonitor();
    auto main_scale = 1.0f;
    if (primary_monitor != nullptr) {
        main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(primary_monitor);
    }

    window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "NPK", nullptr, nullptr);
    if (window == nullptr) {
        std::println(std::cerr, "NPK Error: Failed to create window");
        abort();
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSwapBuffers(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImNodes::CreateContext();
    auto& imgui_io = ImGui::GetIO();
    auto& imnodes_io = ImNodes::GetIO();
    imgui_io.IniFilename = nullptr;
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    imnodes_io.LinkDetachWithModifierClick.Modifier = &imgui_io.KeyCtrl;
    imnodes_io.MultipleSelectModifier.Modifier = &imgui_io.KeyCtrl;

    ImGui::StyleColorsDark();
    auto& imgui_style = ImGui::GetStyle();
    auto& imnodes_style = ImNodes::GetStyle();
    imgui_style.ScaleAllSizes(main_scale);
    imgui_style.FontScaleDpi = main_scale;
    imnodes_style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(nullptr);

    node_module_init();
    image_module_init();
}

application::~application() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImNodes::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    NFD::Quit();
}

auto application::render(std::function<void(void)>&& func) -> void {
    while (!glfwWindowShouldClose(window)) {
        glfwWaitEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        { func(); }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

} // namespace npk
