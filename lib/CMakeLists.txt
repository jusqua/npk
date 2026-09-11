cmake_minimum_required(VERSION 3.17)

if(CMAKE_C_COMPILER)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
file(WRITE ${CMAKE_BINARY_DIR}/.gitignore "*")
file(WRITE ${CMAKE_BINARY_DIR}/.gdignore)

find_package(Python3 REQUIRED)
find_program(GIT git REQUIRED)

include(FetchContent)
set(GODOTCPP_API_VERSION "4.7")
set(GODOTCPP_VERSION "10.0.0-rc2")
FetchContent_Declare(
    godot-cpp
    URL https://github.com/godotengine/godot-cpp/archive/refs/tags/${GODOTCPP_VERSION}.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    SYSTEM
)
FetchContent_MakeAvailable(godot-cpp)

# When compiling for emscripten, this toolchainhack needs to be included.
# set(CMAKE_PROJECT_${PROJECT_NAME}_INCLUDE ${godot-cpp_SOURCE_DIR}/cmake/emsdkHack.cmake)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/godot-cpp/gen/include")

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${godot-cpp_SOURCE_DIR}/cmake")
include(GodotCPPModule)

get_target_property(GODOTCPP_SUFFIX godot::cpp GODOTCPP_SUFFIX)
get_target_property(GODOTCPP_PLATFORM godot::cpp GODOTCPP_PLATFORM)

project(npk LANGUAGES CXX)
add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME}
    PRIVATE
    src/register_types.cpp
    src/register_types.h
    src/example_class.cpp
    src/example_class.h
)

# Fetch a list of the xml files to use for documentation and add to our target
file(GLOB_RECURSE DOC_XML LIST_DIRECTORIES NO CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.xml")

# conditionally add doc data to compile output
if(DOC_XML)
    if(GODOTCPP_TARGET MATCHES "editor|template_debug")
        target_doc_sources(${PROJECT_NAME} "${DOC_XML}")
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE godot-cpp)

# Require at least C++17 for this target
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}>"
    OUTPUT_NAME "${PROJECT_NAME}${GODOTCPP_SUFFIX}"
)

set(TARGET_GDEXTENSION_FILE "${PROJECT_NAME}.gdextension")
configure_file(
    "${PROJECT_SOURCE_DIR}/gdextension.in"
    "${PROJECT_BINARY_DIR}/${TARGET_GDEXTENSION_FILE}"
)

set(GODOT_PROJECT_BINARY_DIR "${PROJECT_SOURCE_DIR}/../addons/${PROJECT_NAME}")

add_custom_target(setup_addon ALL)
add_custom_command(TARGET setup_addon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "$<TARGET_FILE:${PROJECT_NAME}>"
    "${GODOT_PROJECT_BINARY_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy
    "${PROJECT_BINARY_DIR}/${TARGET_GDEXTENSION_FILE}"
    "${GODOT_PROJECT_BINARY_DIR}/${TARGET_GDEXTENSION_FILE}"
    COMMAND ${CMAKE_COMMAND} -E echo '*' >
    "${GODOT_PROJECT_BINARY_DIR}/.gitignore"
)
