cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(BetterCrashlogs VERSION 1.0.0)

# Enable C++ exceptions for Clang-cl
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Xclang -fcxx-exceptions)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.hpp")
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Include GLFW
add_subdirectory(thirdparty/glfw)
target_include_directories(${PROJECT_NAME} PRIVATE "thirdparty/glfw/include")

# Include ImGui
add_library(
    imgui STATIC
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_opengl3.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/imgui ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends)
target_link_libraries(imgui PUBLIC opengl32 glfw)

# Include Zydis
add_subdirectory(thirdparty/zydis)

# Link libraries
target_link_libraries(${PROJECT_NAME} imgui glfw Zydis)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

# Copy CodegenData.txt to resources on build
#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    ${CMAKE_BINARY_DIR}/bindings/bindings/Geode/CodegenData.txt
#    ${CMAKE_SOURCE_DIR}/resources/CodegenData.txt
#)