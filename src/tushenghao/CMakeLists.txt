cmake_minimum_required(VERSION 3.16)

project(mark_detector)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mark_detector
    detector.cpp
)

add_executable(marker_app
    main.cpp
)

target_link_libraries(marker_app
    PRIVATE
    mark_detector
)

target_link_libraries(mark_detector
    PUBLIC
    ${OpenCV_LIBS}
)

enable_testing()  # 开测试功能

add_executable(config_contract_test
    tests/config_contract_test.cpp
)        # 编出测试程序

add_test(
    NAME config_contract_test                  # 注册给 ctest
    COMMAND config_contract_test            # "有个测试叫这名，运行这个命令"
)