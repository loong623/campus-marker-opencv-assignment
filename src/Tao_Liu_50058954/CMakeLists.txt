cmake_minimum_required(VERSION 3.16)

project(tliu154)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(tliu154
    src/main.cpp
)

target_link_libraries(tliu154 PRIVATE
    ${OpenCV_LIBS}
)

target_include_directories(tliu154 PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)