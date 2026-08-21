cmake_minimum_required(VERSION 3.20)
project(SausageManCheat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# --- ImGui 源码（直接编译，不依赖外部库） ---
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui.cpp")
    message(FATAL_ERROR "缺少 vendor/imgui，先运行 scripts/setup_imguibuild")
endif()

file(GLOB IMGUI_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_tables.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_dx11.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_win32.cpp"
)

file(GLOB CHEAT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SDK/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Cheat/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Render/*.cpp"
)

add_library(SausageManCheat SHARED ${IMGUI_SRC} ${CHEAT_SRC})

target_include_directories(SausageManCheat PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends"
)

target_compile_definitions(SausageManCheat PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
)

target_link_libraries(SausageManCheat PRIVATE
    d3d11
    dxgi
    d3dcompiler
    winmm
    user32
    gdi32
)

set_target_properties(SausageManCheat PROPERTIES
    PREFIX ""
    OUTPUT_NAME "SausageManCheat"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin/$<CONFIG>"
)

# 日志输出文件（Windows 调试用）
add_custom_command(TARGET SausageManCheat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Built: $(TargetPath)"
)