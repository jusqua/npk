cmake_minimum_required(VERSION 3.11)
project(npk LANGUAGES CXX)

include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/.gitignore" CONTENT "*\n")
endif()

find_package(OpenGL REQUIRED)

set(GLFW_COMMIT d9d6f0f1f967807ffade6598ea9a631ebaf37a56)
FetchContent_Declare(
  glfw
  URL https://github.com/glfw/glfw/archive/${GLFW_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(glfw)

set(IMGUI_COMMIT 3bae66c735670619baf51391eba7f3d90a25d125)
FetchContent_Declare(
  imgui
  URL https://github.com/ocornut/imgui/archive/${IMGUI_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_Populate(imgui)

set(IMNODES_COMMIT 9de0b5c04218a2d35410774ee1ca637f4352198a)
FetchContent_Declare(
  imnodes
  URL https://github.com/sthagen/Nelarius-imnodes/archive/${IMNODES_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_Populate(imnodes)

add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON)
target_link_libraries(${PROJECT_NAME} PRIVATE
  # Backends
  glfw
  OpenGL::GL
)
target_sources(${PROJECT_NAME} PRIVATE
  # Npk
  main.cpp
  # Dear ImGui
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui.h
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_internal.h
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imstb_rectpack.h
  ${imgui_SOURCE_DIR}/imstb_textedit.h
  ${imgui_SOURCE_DIR}/imstb_truetype.h
  # Dear ImGui Backends
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
  # Nelarius ImNodes
  ${imnodes_SOURCE_DIR}/imnodes.cpp
  ${imnodes_SOURCE_DIR}/imnodes.h
  ${imnodes_SOURCE_DIR}/imnodes_internal.h
)
target_include_directories(${PROJECT_NAME} PUBLIC
  # Dear ImGui
  ${imgui_SOURCE_DIR}
  # Dear ImGui Backends
  ${imgui_SOURCE_DIR}/backends
  # Nelarius ImNodes
  ${imnodes_SOURCE_DIR}
)
