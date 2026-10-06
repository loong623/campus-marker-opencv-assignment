set(CMAKE_CURRENT_SOURCE_DIR "/home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/tools/block3_fixture")
file(GLOB_RECURSE MEASURE_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../app/*.cpp"
)
file(GLOB_RECURSE MEASURE_TOOL_SOURCES  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(APPEND MEASURE_SOURCES ${MEASURE_TOOL_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/common/support_pixel_count.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/common/file_digest.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test_support/block3_fixture.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeLists.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
)
list(SORT MEASURE_SOURCES)
set(MEASURE_DIGEST "")
foreach(MEASURE_FILE IN LISTS MEASURE_SOURCES)
    file(SHA256 "${MEASURE_FILE}" MEASURE_FILE_HASH)
    string(APPEND MEASURE_DIGEST "${MEASURE_FILE_HASH}")
endforeach()
string(SHA256 MEASURE_CODE_HASH "${MEASURE_DIGEST}")
file(WRITE "/home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/cmake-hash-inputs-final-review.txt" "${MEASURE_SOURCES}")
file(WRITE "/home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/cmake-code-hash-final-review.txt" "${MEASURE_CODE_HASH}")
