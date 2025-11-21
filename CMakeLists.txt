cmake_minimum_required(VERSION 3.16)

# ----------------------------------------------------------
# 项目配置 / Project configuration
# ----------------------------------------------------------
project(TestForest
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# 输出到 build/bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ----------------------------------------------------------
# 路径布局（按你的真实项目结构）
# ----------------------------------------------------------
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

# ----------------------------------------------------------
# 可执行目标
# ----------------------------------------------------------
add_executable(test_forest_bench
    ${TEST_FOREST_SOURCES}
    ${TEST_FOREST_HEADERS}   # 仅用于 IDE 展示
)

# include 目录（注意：要 include src/proj/headers）
target_include_directories(test_forest_bench
    PRIVATE
        "${PROJ_ROOT}/headers"
)

# ----------------------------------------------------------
# 编译选项
# ----------------------------------------------------------
if(MSVC)
    target_compile_options(test_forest_bench
        PRIVATE
            /W4
            /permissive-
            /EHsc
    )
else()
    target_compile_options(test_forest_bench
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

# ----------------------------------------------------------
# filesystem 兼容性（老 GCC 才需要）
# ----------------------------------------------------------
# if(NOT MSVC)
#     target_link_libraries(test_forest_bench PRIVATE stdc++fs)
# endif()
