cmake_minimum_required(VERSION 3.16)

project(mark_detector)

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

enable_testing()

add_executable(config_contract_test
    tests/config_contract_test.cpp
)

add_test(
    NAME config_contract_test
    COMMAND config_contract_test
)