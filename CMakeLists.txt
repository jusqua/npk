cmake_minimum_required(VERSION 3.11)
project(npk LANGUAGES CXX C)

include(FetchContent)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# STL is complaining with AdaptiveCpp runtime iterators,
# but it's working fine in Release builds.
# Disabling iterator debug level until it's fixed.
add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if (NOT CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if(NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/.gitignore" CONTENT "*\n")
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

find_package(OpenGL REQUIRED)
find_package(AdaptiveCpp REQUIRED CONFIG)

# Dependency for libpng
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_SHARED ${BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)
set(ZLIB_INSTALL ${BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)
set(ZLIB_COMMIT da607da739fa6047df13e66a2af6b8bec7c2a498) # 1.3.2
FetchContent_Declare(
  zlib
  URL  https://github.com/madler/zlib/archive/${ZLIB_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(zlib)
add_library(ZLIB::ZLIB ALIAS zlibstatic)
set(ZLIB_ROOT "${zlib_SOURCE_DIR}" CACHE INTERNAL "")
set(ZLIB_LIBRARY ZLIB::ZLIB CACHE INTERNAL "" FORCE)
set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR};${zlib_BINARY_DIR}" CACHE INTERNAL "" FORCE)

# File picker
set(NFD_X11 OFF)
set(NFD_WAYLAND ON)
set(NFD_PORTAL ON)
set(NFD_COMMIT 5381ddbf0b33d3b7b4c5679b0e796d77848a158d) # 1.4.0
FetchContent_Declare(
  nfd
  GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
  GIT_TAG ${NFD_COMMIT}
  GIT_SHALLOW ON
  GIT_SUBMODULES_RECURSE ON
)
FetchContent_MakeAvailable(nfd)

# Window handler
set(GLFW_COMMIT d9d6f0f1f967807ffade6598ea9a631ebaf37a56) # 3.5.1
FetchContent_Declare(
  glfw
  URL https://github.com/glfw/glfw/archive/${GLFW_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(glfw)

# PNG image loader
set(PNG_SHARED ${BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
set(LIBPNG_COMMIT 4eeb825ab26605ed34cd74495d4cce920939fe9f)
FetchContent_Declare(
  libpng
  URL https://github.com/pnggroup/libpng/archive/${LIBPNG_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(libpng)

# TIFF image loader
set(tiff-tools OFF)
set(tiff-tests OFF)
set(tiff-contrib OFF)
set(tiff-docs OFF)
set(LIBTIFF_COMMIT d23249978a7a99ea2e0752a476611914501577de)
FetchContent_Declare(
  libtiff
  GIT_REPOSITORY https://gitlab.com/libtiff/libtiff.git
  GIT_TAG ${LIBTIFF_COMMIT}
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(libtiff)

set(IMGUI_COMMIT 3bae66c735670619baf51391eba7f3d90a25d125)
FetchContent_Declare(
  imgui
  URL https://github.com/ocornut/imgui/archive/${IMGUI_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  SOURCE_SUBDIR .cmake-ignore
)
FetchContent_MakeAvailable(imgui)

set(IMNODES_COMMIT 9de0b5c04218a2d35410774ee1ca637f4352198a)
FetchContent_Declare(
  imnodes
  URL https://github.com/sthagen/Nelarius-imnodes/archive/${IMNODES_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  SOURCE_SUBDIR .cmake-ignore
)
FetchContent_MakeAvailable(imnodes)

set(HOK_COMMIT main)
FetchContent_Declare(
  hok
  GIT_REPOSITORY https://github.com/jusqua/hok
  GIT_TAG ${HOK_COMMIT}
  SOURCE_SUBDIR .cmake-ignore
)
FetchContent_MakeAvailable(hok)

add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON)
target_link_libraries(${PROJECT_NAME} PRIVATE
  # Package dependencies
  ZLIB::ZLIB
  # File picker
  nfd
  # Backends
  glfw
  OpenGL::GL
  # Image loaders
  png_static
  tiffxx
)
target_sources(${PROJECT_NAME} PRIVATE
  # Npk
  utils.cpp
  utils.hpp
  application.cpp
  application.hpp
  processing.cpp
  processing.hpp
  image.cpp
  image.hpp
  color.cpp
  color.hpp
  attribute.cpp
  attribute.hpp
  node.cpp
  node.hpp
  link.cpp
  link.hpp
  graph.cpp
  graph.hpp
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
