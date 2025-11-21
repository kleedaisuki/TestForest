cmake_minimum_required(VERSION 3.16)

# ==========================================================
# 项目配置 / Project configuration
# ==========================================================
project(TestForest
    VERSION 0.1.0
    LANGUAGES CXX
)

# 使用 C++17 / Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==========================================================
# 构建类型：Debug / Release
#   - 单配置生成器（MinGW Makefiles, Ninja, Unix Makefiles）使用 CMAKE_BUILD_TYPE
#   - 多配置生成器（Visual Studio, Xcode）使用 CMAKE_CONFIGURATION_TYPES
# ==========================================================
if(NOT CMAKE_CONFIGURATION_TYPES) # single-config generators
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug or Release)" FORCE)
    endif()
endif()

# 统一可执行文件输出目录 / Put executables into build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ==========================================================
# 路径布局
#   src/proj/
#     ├─ headers/*.hpp
#     ├─ src/utils.cpp
#     └─ main.cpp
# ==========================================================
set(PROJ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src/proj")

set(TEST_FOREST_HEADERS
    "${PROJ_ROOT}/headers/utils.hpp"
    "${PROJ_ROOT}/headers/Binary-Tree.hpp"
    "${PROJ_ROOT}/headers/B-Tree.hpp"
    "${PROJ_ROOT}/headers/AVL-Tree.hpp"
    "${PROJ_ROOT}/headers/Red-Black-Tree.hpp"
)

set(TEST_FOREST_SOURCES
    "${PROJ_ROOT}/src/utils.cpp"
    "${PROJ_ROOT}/main.cpp"
)

# ==========================================================
# 可执行目标 / Executable target
# ==========================================================
add_executable(test_forest_bench
    ${TEST_FOREST_SOURCES}
    ${TEST_FOREST_HEADERS}  # 仅用于 IDE 展示 / headers for IDE only
)

# 头文件搜索路径：src/proj/headers
target_include_directories(test_forest_bench
    PRIVATE
        "${PROJ_ROOT}/headers"
)

# ==========================================================
# 编译选项（按 Debug / Release 区分，兼容 MSVC 与 GCC/Clang）
#   - Debug:
#       * MSVC: /Od /Zi + /Wall（很凶的全部警告）
#       * GCC/Clang: -Og -g + 各种 W*（尽可能多）
#   - Release:
#       * MSVC: /O2 + /w 禁用警告
#       * GCC/Clang: -O2 + -w 禁用警告
#
#   使用 generator expression:
#       $<$<CONFIG:Debug>:...>
#       $<$<CONFIG:Release>:...>
#
#   单配置生成器时 CONFIG 等于 CMAKE_BUILD_TYPE。
# ==========================================================
if(MSVC)

    target_compile_options(test_forest_bench PRIVATE
        #---------------------- Debug 配置 ----------------------
        $<$<CONFIG:Debug>:
            /Od             # 优化关闭，便于调试（近似 -Og）
            /Zi             # 调试信息 / debug information
            /W4             # 较高警告等级
            /Wall           # MSVC 下开启几乎所有警告
            /permissive-    # 更接近标准 C++
            /EHsc           # 标准异常模型
        >
        #---------------------- Release 配置 --------------------
        $<$<CONFIG:Release>:
            /O2             # 优化等级 / optimization
            /w              # 禁用所有警告
            /EHsc
        >
    )

else()  # GCC / Clang（含 MinGW-w64）

    target_compile_options(test_forest_bench PRIVATE
        #---------------------- Debug 配置 ----------------------
        $<$<CONFIG:Debug>:
            -Og
            -g
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wold-style-cast
            -Wdouble-promotion
            -Wnull-dereference
            -Wformat=2
            -Wimplicit-fallthrough
            -Woverloaded-virtual
            -Wnon-virtual-dtor
            -Wcast-align
            -Wcast-qual
        >
        #---------------------- Release 配置 --------------------
        $<$<CONFIG:Release>:
            -O2
            -w              # 禁用所有警告 / disable all warnings
        >
    )

endif()

# ==========================================================
# Filesystem 兼容性的说明（GCC 9+ / MSYS2 MinGW 通常不需要 stdc++fs）
#
#   - 现代 GCC / Clang / MSVC：
#       std::filesystem 已经在主标准库中，无需额外链接。
#   - 只有在非常老的 GCC 上遇到 undefined reference 时，
#       再考虑解开下面的链接选项。
# ==========================================================
# if(NOT MSVC)
#     target_link_libraries(test_forest_bench PRIVATE stdc++fs)
# endif()

# ==========================================================
# 友好提示：在 Windows + MinGW-w64 环境下推荐使用：
#
#   cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
#   cmake --build .
#
# 在 Visual Studio 环境下推荐使用：
#
#   cmake -G "Visual Studio 17 2022" -A x64 ..
#   cmake --build . --config Debug
#
# 具体生成器需要在命令行指定，本文件只负责根据编译器类型
#   （MSVC vs GCC/Clang）和配置（Debug/Release）自动选择 flags。
# ==========================================================
