cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(Breakdown VERSION 2.0.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
set(MACOS_SOURCES "")
if (APPLE)
    enable_language(OBJCXX)
    file(GLOB_RECURSE MACOS_SOURCES CONFIGURE_DEPENDS src/*.mm)
endif()
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${MACOS_SOURCES})

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# wxWidgets
CPMAddPackage("gh:wxWidgets/wxWidgets@3.2.6")
target_link_libraries(${PROJECT_NAME} wx::core wx::base)

setup_geode_mod(${PROJECT_NAME})
