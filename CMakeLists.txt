cmake_minimum_required(VERSION 3.11)
project(npk LANGUAGES CXX C)

include(FetchContent)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if (NOT CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

if(NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/.gitignore" CONTENT "*\n")
endif()

set(PREFER_BACKEND "Auto" CACHE STRING "Select backend (Auto, IntelSYCL, AdaptiveCpp)")
set_property(CACHE PREFER_BACKEND PROPERTY STRINGS "Auto" "IntelSYCL" "AdaptiveCpp")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(PREFER_BACKEND STREQUAL "IntelSYCL")
    find_package(IntelSYCL CONFIG REQUIRED)
elseif(PREFER_BACKEND STREQUAL "AdaptiveCpp")
    find_package(AdaptiveCpp CONFIG REQUIRED)
else() # Try to find SYCL compilers packages (It can be gone wrong)
    find_package(IntelSYCL CONFIG QUIET)
    if(IntelSYCL_FOUND)
        message(STATUS "Using IntelSYCL")
    else()
        find_package(AdaptiveCpp CONFIG QUIET)
        if (AdaptiveCpp_FOUND)
            message(STATUS "Using AdaptiveCpp")
        else()
            message(FATAL_ERROR "Neither IntelSYCL nor AdaptiveCpp could be found")
        endif()
    endif()
endif()

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

set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "" FORCE) # Unused
set(SDL_VIDEO ON CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE) # Unused
set(SDL_RENDER ON CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE) # Unused
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE) # Unused
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE) # Unused
set(SDL_HIDAPI ON CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE) # Unused
set(SDL_SENSOR OFF CACHE BOOL "" FORCE) # Unused
set(SDL_DIALOG ON CACHE BOOL "" FORCE)
set(SDL_TRAY OFF CACHE BOOL "" FORCE) # Unused
set(SDL_COMMIT f00dc919ca35b9756316e86c4777392de442f9b2) # 3.4.20
FetchContent_Declare(
  sdl
  URL https://github.com/libsdl-org/SDL/archive/${SDL_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(sdl)

set(IMGUI_COMMIT d3b36a875f0f7e7446b99606ca3d6df27b7dad7a)
FetchContent_Declare(
  imgui
  URL https://github.com/ocornut/imgui/archive/${IMGUI_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  SOURCE_SUBDIR .cmake-ignore
)
FetchContent_MakeAvailable(imgui)

set(IMNODES_COMMIT 2d0c280b07b2f62b120034811cc6e41251673961)
FetchContent_Declare(
  imnodes
  URL https://github.com/Nelarius/imnodes/archive/${IMNODES_COMMIT}.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  SOURCE_SUBDIR .cmake-ignore
)
FetchContent_MakeAvailable(imnodes)

add_library(imgui_lib STATIC
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
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.h
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.h
    # Nelarius ImNodes
    ${imnodes_SOURCE_DIR}/imnodes.cpp
    ${imnodes_SOURCE_DIR}/imnodes.h
    ${imnodes_SOURCE_DIR}/imnodes_internal.h
)
target_include_directories(imgui_lib PUBLIC
  # Dear ImGui
  ${imgui_SOURCE_DIR}
  # Dear ImGui Backends
  ${imgui_SOURCE_DIR}/backends
  # Nelarius ImNodes
  ${imnodes_SOURCE_DIR}
)
target_link_libraries(imgui_lib PRIVATE
    SDL3::SDL3
)

set(HOK_COMMIT main)
FetchContent_Declare(
  hok
  GIT_REPOSITORY https://github.com/jusqua/hok
  GIT_TAG ${HOK_COMMIT}
  SOURCE_SUBDIR .cmake-ignore
)
FetchContent_MakeAvailable(hok)

add_library(hok_lib INTERFACE)
target_include_directories(hok_lib INTERFACE ${hok_SOURCE_DIR})

add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON)
target_link_libraries(${PROJECT_NAME} PRIVATE
  # Image loaders
  png_static
  tiffxx
  # Windowing
  imgui_lib
  SDL3::SDL3
  # Processing
  hok_lib
)
target_sources(${PROJECT_NAME} PRIVATE
  utils.cpp
  utils.hpp
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
  main.hpp
  main.cpp
)
add_sycl_to_target(TARGET ${PROJECT_NAME} SOURCES processing.cpp)
