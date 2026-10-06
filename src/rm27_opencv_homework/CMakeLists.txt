cmake_minimum_required(VERSION 3.16)
project(ArmorDetector VERSION 1.0.0 LANGUAGES CXX)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认 Release，调试时手动切 Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# 选项：是否启用 OpenCV trackbar 调参（需要 GUI 环境）
# WSL 下 GUI 经常出问题，默认关闭，推荐用 YAML 热重载调参。
option(USE_TRACKBAR "Enable OpenCV trackbar for parameter tuning" OFF)

if(USE_TRACKBAR)
    add_compile_definitions(USE_TRACKBAR)
endif()

# 编译选项
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
)

# 查找依赖
find_package(OpenCV 4 REQUIRED)
find_package(yaml-cpp REQUIRED)

# 头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

# 源文件列表
set(SOURCES
    src/capture.cpp
    src/tuner.cpp
    src/video_writer.cpp
    src/params.cpp
    src/image_utils.cpp
    src/light_bar.cpp
    src/armor.cpp
    src/detector.cpp
    src/debug.cpp
    src/main.cpp
)

# 主可执行文件
add_executable(armor_detector ${SOURCES})

target_link_libraries(armor_detector
    ${OpenCV_LIBS}
    yaml-cpp
)

# 测试可执行文件（占位）
add_executable(armor_detector_tests tests/test_placeholder.cpp)
target_link_libraries(armor_detector_tests
    ${OpenCV_LIBS}
    yaml-cpp
)
