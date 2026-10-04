/*最小实现目标：
DetectionResult
        |
        v
GeometryMatcher
        |
        v
MarkerGeometry.polygons
*/
#include "geometry_matcher.hpp"
// 已废弃：方向错误，仅保留作历史参考，勿在此基础上继续开发。
// 本文件假设 ID 已知（ID 查表），但实战中 ID 正是几何推理要求出的结果，
// 不能由检测器直接提供。正确路线见 geometry_types.hpp。
// 待正式链路跑通后，将通过 cleanup commit 删除。


namespace mark
{

    // 构造 GeometryMatcher，保存几何模型引用供后续匹配查询使用。
    // 使用引用而不是复制模型，因为 MarkerGeometry 是统一的数据来源，matcher 只负责查询。
    GeometryMatcher::GeometryMatcher(
        const MarkerGeometry &geometry)
        : geometry_(geometry)
    {
    }

    // 根据检测结果中的 MARK ID，在预定义几何模型中寻找对应 polygon。
    // 当前阶段先通过 ID 完成关联，后续可在此基础上加入几何误差匹配。
    const GeometryPolygon *GeometryMatcher::match(
        const DetectionResult &detection) const
    {
        for (const auto &polygon : geometry_.polygons)
        {
            if (polygon.id == detection.id)
            {
                // 返回已有模型中的地址，避免复制 polygon，并保持模型唯一来源。
                return &polygon;
            }
        }  // MarkerGeometry 管理数据生命周期；GeometryMatcher 只查询；不产生新的 polygon 副本

        // 没有找到对应几何模型时返回 nullptr，交由调用方处理匹配失败。
        return nullptr;
    }

} // namespace mark