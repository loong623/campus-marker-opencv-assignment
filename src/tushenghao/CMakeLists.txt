cmake_minimum_required(VERSION 3.16)

project(mark_detector)

find_package(OpenCV REQUIRED)

find_package(GTest REQUIRED)      # 装了gtest（被迫）

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加库
add_library(mark_detector
    detector.cpp
    preprocess.cpp
    config.cpp
    marker_geometry.cpp
    geometry_utils.cpp
    geometry_matcher.cpp
    geometry_observation.cpp
    geometry_validation.cpp
    corner_resolver.cpp
    screen_order.cpp
    semantic_resolver.cpp
    detection_validator.cpp
)

target_include_directories(mark_detector
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)    # 解决编译期"找不到头文件"（看得见声明）工程实例：config_error_test.cpp include 了 config.hpp，但是 target 没有继承头文件搜索路径

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
)        # 编出测试程序1

add_executable(config_error_test
    tests/config_error_test.cpp
)          # 编出测试程序2

target_link_libraries(config_error_test
    PRIVATE
    mark_detector
)          # 解决链接期"找不到实现"（找得到定义） 工程实例：自动获得 mark_detector 的 PUBLIC include path（接上面）

add_test(
    NAME config_contract_test                  # 注册给 ctest
    COMMAND config_contract_test            # "有个测试叫这名，运行这个命令"
)

add_test(
    NAME config_error_test
    COMMAND config_error_test
)        # 注册配置错误测试

# 配置往返测试（3）
add_executable(config_roundtrip_test tests/config_roundtrip_test.cpp)
target_link_libraries(config_roundtrip_test PRIVATE mark_detector)
add_test(NAME config_roundtrip_test COMMAND config_roundtrip_test)

# 检测器契约测试（4）
add_executable(detector_contract_test tests/detector_contract_test.cpp)
target_link_libraries(detector_contract_test PRIVATE mark_detector)
add_test(NAME detector_contract_test COMMAND detector_contract_test)

# 几何模型加载测试（5）
add_executable(geometry_model_test
    tests/geometry_model_test.cpp
)

target_link_libraries(
    geometry_model_test
    PRIVATE
    mark_detector
)

add_test(
    NAME geometry_model_test
    COMMAND geometry_model_test
)

# 几何计算工具测试（6）
add_executable(geometry_utils_test
    tests/geometry_utils_test.cpp
)

target_link_libraries(geometry_utils_test
    PRIVATE
    mark_detector
)

add_test(
    NAME geometry_utils_test
    COMMAND geometry_utils_test
)

# 几何假设测试（7）
add_executable(
    geometry_matcher_test
    tests/geometry_matcher_test.cpp)


target_link_libraries(
     geometry_matcher_test
     PRIVATE
     mark_detector)


add_test(
    NAME geometry_matcher_test
    COMMAND geometry_matcher_test)

# TODO: 临时手动验证 Step7，赛前用合成器做完整验证后删除
add_executable(manual_validation_check tests/manual_validation_check.cpp)
target_link_libraries(manual_validation_check mark_detector ${OpenCV_LIBS})
add_test(NAME manual_validation_check COMMAND manual_validation_check)

# 角点恢复测试 (Block 3 Step 3)
add_executable(
    corner_resolver_test
    tests/corner_resolver_test.cpp)

target_link_libraries(
    corner_resolver_test
    PRIVATE
    mark_detector
    GTest::gtest
    GTest::gtest_main) # test 用了 <gtest/gtest.h>，得显式链 gtest

add_test(
    NAME corner_resolver_test
    COMMAND corner_resolver_test)

# Step 4 屏幕顺序测试：验证 orderScreenCorners() 的排序、平局、退化拒绝
add_executable(
    screen_order_test
    tests/screen_order_test.cpp
)

target_link_libraries(
    screen_order_test
    PRIVATE
    mark_detector
    GTest::gtest
    GTest::gtest_main
)

add_test(
    NAME screen_order_test
    COMMAND screen_order_test
)

# Step 5 语义归并测试：验证 resolveSemantics() 的几何一致性、方向唯一性、残差选择、截断覆盖
add_executable(semantic_resolver_test tests/semantic_resolver_test.cpp)
target_link_libraries(semantic_resolver_test PRIVATE mark_detector GTest::gtest GTest::gtest_main)
add_test(NAME semantic_resolver_test COMMAND semantic_resolver_test)

# Step 6 几何校验测试：验证 validateDetectionGeometry() 的凸性、自交、映射、越界、退化拒绝
add_executable(detection_validator_test tests/detection_validator_test.cpp)
target_link_libraries(detection_validator_test PRIVATE mark_detector GTest::gtest GTest::gtest_main)
add_test(NAME detection_validator_test COMMAND detection_validator_test)