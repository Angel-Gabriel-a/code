cmake_minimum_required(VERSION 3.10)
project(xlog LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------------
# 收集源文件
# ----------------------------
file(GLOB_RECURSE SRC_FILES src/*.cpp)
file(GLOB_RECURSE TEST_FILES tests/*.cpp)
file(GLOB_RECURSE BENCH_FILES benchmarks/*.cpp)

# ----------------------------
# 编译库
# ----------------------------
add_library(xlog_lib ${SRC_FILES})
target_include_directories(xlog_lib
    PUBLIC ${CMAKE_SOURCE_DIR}/include
)

# ----------------------------
# 编译测试可执行程序
# ----------------------------
if(TEST_FILES)
    add_executable(xlog_tests ${TEST_FILES})
    target_link_libraries(xlog_tests PRIVATE xlog_lib)
    target_include_directories(xlog_tests
        PRIVATE ${CMAKE_SOURCE_DIR}/src
        PUBLIC  ${CMAKE_SOURCE_DIR}/include
    )
endif()

# ----------------------------
# 编译基准可执行程序
# ----------------------------
if(BENCH_FILES)
    add_executable(xlog_bench ${BENCH_FILES})
    target_link_libraries(xlog_bench PRIVATE xlog_lib)
    target_include_directories(xlog_bench
        PRIVATE ${CMAKE_SOURCE_DIR}/src
        PUBLIC  ${CMAKE_SOURCE_DIR}/include
    )
endif()
