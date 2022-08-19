cmake_minimum_required(VERSION 3.5)

project(clock LANGUAGES CXX VERSION 0.1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(src/cmake_defs.h.in include/generated/cmake_defs.h)
include_directories(AFTER ${PROJECT_BINARY_DIR}/include)

add_subdirectory(swal)

set(CMAKE_CXX_FLAGS_DEBUG  ${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic -pedantic-errors)
add_definitions(-D_WIN32_WINNT=0xA00 -DUNICODE=1)

set(CLOCK_GENERATED_DIR ${PROJECT_BINARY_DIR}/generated)
set(CLOCK_SHADER_OBJECT ${CLOCK_GENERATED_DIR}/shaders/hlsl)

set(CLOCK_BUILT_IN_SHADERS
    src/frag.hlsl
    src/vert.hlsl
)

add_custom_command(OUTPUT
    ${CLOCK_SHADER_OBJECT}.cpp
COMMAND
    ${CMAKE_COMMAND}
    -DOUTPUT=${CLOCK_SHADER_OBJECT}
    -P src/pack_shaders_to_cpp.cmake ${CLOCK_BUILT_IN_SHADERS}
DEPENDS
    src/pack_shaders_to_cpp.cmake
    ${CLOCK_BUILT_IN_SHADERS}
BYPRODUCTS
    ${CLOCK_SHADER_OBJECT}.h
WORKING_DIRECTORY
    ${PROJECT_SOURCE_DIR}
)

add_custom_target(shaders SOURCES src/pack_shaders_to_cpp.cmake ${CLOCK_BUILT_IN_SHADERS})

include_directories(${CLOCK_GENERATED_DIR})

add_library(rc_win32 OBJECT
    src/res.h
    src/res.rc
)

add_executable(clock
    $<TARGET_OBJECTS:rc_win32>
    ${CLOCK_SHADER_OBJECT}.cpp
    src/App.cpp
    src/App.h
    src/BasicBitmapLoader.cpp
    src/BasicBitmapLoader.h
    src/DebugTextureProvider.cpp
    src/DebugTextureProvider.h
    src/ITextureDataProvider.h
    src/Widget.cpp
    src/Widget.h
    src/main.cpp
    src/res.h
)
target_compile_options(clock PRIVATE)
target_link_options(clock PRIVATE /subsystem:windows /entry:mainCRTStartup)
target_link_libraries(clock PRIVATE swal::swal dwmapi d3d11 dxgi d3dcompiler)
