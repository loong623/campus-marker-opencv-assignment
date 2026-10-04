// 验证DetectionResult → GeometryMatcher → GeometryPolygon 的数据链是否闭合。
#include "geometry_matcher.hpp"
#include "marker_geometry.hpp"

#include <cassert>
// 已废弃：方向错误，仅保留作历史参考，勿在此基础上继续开发。
// 本文件假设 ID 已知（ID 查表），但实战中 ID 正是几何推理要求出的结果，
// 不能由检测器直接提供。正确路线见 geometry_types.hpp。
// 待正式链路跑通后，将通过 cleanup commit 删除。


int main()
{
    const auto geometry = mark::loadMarkerGeometry(           // debug:ctest working directory 导致相对路径失效
        "../../src/tushenghao/config/marker_geometry.yaml");  // 相对路径

    mark::GeometryMatcher matcher(geometry);

    // 测试存在的 polygon 可以匹配（返回非空，id 对得上）
    mark::DetectionResult detection;
    detection.id = "L0";

    const auto *polygon = matcher.match(detection);

    assert(polygon != nullptr);
    assert(polygon->id == "L0");

    // 测试不存在的 polygon 返回 nullptr（不崩，不瞎编）
    mark::DetectionResult unknown;
    unknown.id = "UNKNOWN";

    assert(matcher.match(unknown) == nullptr);

    return 0;
}