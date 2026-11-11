#include "main.hpp"
#include <print>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imnodes.h>
#include <SDL3/SDL.h>

#include "graph.hpp"
#include "image.hpp"
#include "node.hpp"

static SDL_Renderer* s_renderer;

namespace npk {

// TODO: Change the way the app provides globals
auto get_application_renderer() -> SDL_Renderer* {
    return s_renderer;
}

} // namespace npk

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        std::println("Error: SDL_Init(): {}", SDL_GetError());
        return 1;
    }

    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("NPK", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr) {
        std::println("Error: SDL_CreateWindow(): {}", SDL_GetError());
        return 1;
    }
    s_renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(s_renderer, 1);
    if (s_renderer == nullptr) {
        SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImNodes::CreateContext();
    auto& imgui_io = ImGui::GetIO();
    auto& imnodes_io = ImNodes::GetIO();
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    imgui_io.IniFilename = nullptr;
    imgui_io.LogFilename = nullptr;
    imnodes_io.LinkDetachWithModifierClick.Modifier = &imgui_io.KeyCtrl;
    imnodes_io.MultipleSelectModifier.Modifier = &imgui_io.KeyCtrl;

    auto& imgui_style = ImGui::GetStyle();
    auto& imnodes_style = ImNodes::GetStyle();
    ImGui::StyleColorsDark();
    ImNodes::StyleColorsDark();
    imgui_style.FrameRounding = 3.0f;
    imgui_style.GrabRounding = 3.0f;
    imgui_style.ScaleAllSizes(main_scale);
    imgui_style.FontScaleDpi = main_scale;
    imnodes_style.Flags |= ImNodesStyleFlags_GridLinesPrimary | ImNodesStyleFlags_GridSnapping;

    ImGui_ImplSDL3_InitForSDLRenderer(window, s_renderer);
    ImGui_ImplSDLRenderer3_Init(s_renderer);

    npk::node_module_init();
    npk::image_module_init();
    auto graph = npk::graph();

    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        graph.render();

        ImGui::Render();
        SDL_SetRenderScale(s_renderer, imgui_io.DisplayFramebufferScale.x, imgui_io.DisplayFramebufferScale.y);
        SDL_RenderClear(s_renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), s_renderer);
        SDL_RenderPresent(s_renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    ImNodes::DestroyContext();

    SDL_DestroyRenderer(s_renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
