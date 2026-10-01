cmake_minimum_required(VERSION 3.11)
project(npk LANGUAGES CXX)

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/.gitignore" CONTENT "*\n")
endif()

find_package(OpenGL REQUIRED)
find_package(AdaptiveCpp REQUIRED CONFIG)

# Dependency for libpng
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZLIB_COMMIT da607da739fa6047df13e66a2af6b8bec7c2a498) # 1.3.2
FetchContent_Declare(
  zlib
  URL  https://github.com/madler/zlib/archive/${ZLIB_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(zlib)
set(ZLIB_ROOT "${zlib_SOURCE_DIR}" CACHE INTERNAL "")
set(ZLIB_LIBRARY ZLIB::ZLIB CACHE INTERNAL "" FORCE)
set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR};${zlib_BINARY_DIR}" CACHE INTERNAL "" FORCE)

# Window handler
set(GLFW_COMMIT d9d6f0f1f967807ffade6598ea9a631ebaf37a56) # 3.5.1
FetchContent_Declare(
  glfw
  URL https://github.com/glfw/glfw/archive/${GLFW_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(glfw)

# PNG image loader
set(LIBPNG_COMMIT 4eeb825ab26605ed34cd74495d4cce920939fe9f)
FetchContent_Declare(
  libpng
  URL https://github.com/pnggroup/libpng/archive/${LIBPNG_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(libpng)

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

set(HOK_COMMIT main)
FetchContent_Declare(
  hok
  URL https://github.com/jusqua/hok/archive/${HOK_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_Populate(hok)

add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON)
target_link_libraries(${PROJECT_NAME} PRIVATE
  # Package dependencies
  ZLIB::ZLIB
  # Backends
  glfw
  OpenGL::GL
  # Image loaders
  png_static
)
target_sources(${PROJECT_NAME} PRIVATE
  # Npk
  main.cpp
  application.cpp
  application.hpp
  processing.cpp
  processing.hpp
  image.cpp
  image.hpp
  color.cpp
  color.hpp
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
  # HOK
  ${hok_SOURCE_DIR}/hok.hpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
  # Dear ImGui
  ${imgui_SOURCE_DIR}
  # Dear ImGui Backends
  ${imgui_SOURCE_DIR}/backends
  # Nelarius ImNodes
  ${imnodes_SOURCE_DIR}
  # HOK
  ${hok_SOURCE_DIR}
)
add_sycl_to_target(TARGET ${PROJECT_NAME})
