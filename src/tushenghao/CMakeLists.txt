cmake_minimum_required(VERSION 3.16)
project(mark_detector LANGUAGES CXX)
find_package(OpenCV REQUIRED)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 六个内部源码模块仍组成一个库；公开头与内部头的搜索路径分开。
add_library(mark_detector
    lib/core/marker_geometry.cpp
    lib/core/geometry_utils.cpp
    lib/config/config.cpp
    lib/preprocess/preprocess.cpp
    lib/geometry/geometry_matcher.cpp
    lib/geometry/geometry_observation.cpp
    lib/geometry/geometry_l_topology.cpp
    lib/geometry/geometry_validation.cpp
    lib/geometry/geometry_assignment_completion.cpp
    lib/corners/corner_resolver.cpp
    lib/corners/corner_observation.cpp
    lib/corners/corner_edge_fit.cpp
    lib/corners/corner_evidence_validation.cpp
    lib/corners/screen_order.cpp
    lib/corners/semantic_resolver.cpp
    lib/corners/detection_validator.cpp
    lib/pipeline/decode_stage.cpp
    lib/pipeline/detector.cpp
)
target_include_directories(mark_detector
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)
target_link_libraries(mark_detector PUBLIC ${OpenCV_LIBS})

# 仅工程内app/测试/审计显式选择内部头，不将lib路径发布给普通Detector消费者。
add_library(mark_detector_internal_headers INTERFACE)
target_include_directories(mark_detector_internal_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
target_link_libraries(mark_detector_internal_headers INTERFACE mark_detector)
add_library(mark_test_support INTERFACE)
target_include_directories(mark_test_support INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/test_support")

add_executable(marker_app app/main.cpp)
target_link_libraries(marker_app PRIVATE mark_detector_internal_headers)

enable_testing()
# 保持原16个CTest程序；新增Block3主动检查在Release也会报告失败。
set(MARK_TEST_NAMES
    block3_contract_regression_test
    assignment_completion_test
    corner_edge_fit_test
    config_contract_test
    config_error_test
    config_roundtrip_test
    detector_contract_test
    geometry_model_test
    geometry_utils_test
    geometry_matcher_test
    manual_validation_check
    corner_resolver_test
    screen_order_test
    semantic_resolver_test
    detection_validator_test
    geometry_l_topology_test
)
foreach(MARK_TEST_NAME IN LISTS MARK_TEST_NAMES)
    add_executable(${MARK_TEST_NAME} "tests/${MARK_TEST_NAME}.cpp")
    target_link_libraries(${MARK_TEST_NAME} PRIVATE mark_detector_internal_headers mark_test_support)
    add_test(NAME ${MARK_TEST_NAME} COMMAND ${MARK_TEST_NAME})
endforeach()

# 摘要/OpenSSL只属于审计可执行程序，Detector库不引入JSON/OpenSSL。
find_package(OpenSSL REQUIRED COMPONENTS Crypto)
add_executable(decode_audit tools/audit/decode_audit.cpp)
target_include_directories(decode_audit PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tools")
target_link_libraries(decode_audit PRIVATE mark_detector_internal_headers OpenSSL::Crypto)
execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE MARK_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE)
# 覆盖搬迁后的所有生产源/头及审计实现，不再只扫描根目录。
file(GLOB_RECURSE MARK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/*.cpp"
)
list(APPEND MARK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit/decode_audit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/common/file_digest.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
)
list(SORT MARK_SOURCES)
set(MARK_DIGEST "")
foreach(MARK_FILE IN LISTS MARK_SOURCES)
    file(SHA256 "${MARK_FILE}" MARK_FILE_HASH)
    string(APPEND MARK_DIGEST "${MARK_FILE_HASH}")
endforeach()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${MARK_SOURCES})
string(SHA256 MARK_CODE_HASH "${MARK_DIGEST}")
target_compile_definitions(decode_audit PRIVATE MARK_DEFAULT_CONFIG="${CMAKE_CURRENT_SOURCE_DIR}/config/detector.yaml" MARK_COMMIT="${MARK_COMMIT}" MARK_CODE_HASH="${MARK_CODE_HASH}")
