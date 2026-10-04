// Geometry primitive integration（衔接）
// （拆包裹的人）GeometryMatcher：活的，是个类，有 match() 方法。它拿到 DetectionResult，去模型库里查，判断"这个检测结果到底对应哪个几何图形"。
// 先把数据流打通
// // GeometryMatcher 根据检测结果，在预定义几何模型中寻找对应 polygon。
//  matcher 是"翻译官"，左手拿检测结果，右手翻模型词典，输出"这个检测对应图纸上的谁"。
#pragma once
// 已废弃：方向错误，仅保留作历史参考，勿在此基础上继续开发。
// 本文件假设 ID 已知（ID 查表），但实战中 ID 正是几何推理要求出的结果，
// 不能由检测器直接提供。正确路线见 geometry_types.hpp。
// 待正式链路跑通后，将通过 cleanup commit 删除。


#include "marker_geometry.hpp"
#include "detection_result.hpp"

namespace mark
{

    // 根据检测结果，将视觉结果关联到预定义几何模型。
    class GeometryMatcher
    {
    public:
        explicit GeometryMatcher(
            const MarkerGeometry &geometry);   // 把模型交给 matcher（构造时）matcher 内部存着模型的引用，干活时查表用。

        // 根据检测结果寻找对应 polygon。
        // 返回 nullptr 表示匹配失败。（指针好处：可以表示“没有找到”这个概念）
        const GeometryPolygon *match(
            const DetectionResult &detection) const;  // 调用时const GeometryPolygon* p = matcher.match(detection);（省内存）
            // 输入 DetectionResult（检测器看到的东西），输出 GeometryPolygon*（模型库里对应的图纸），找不到返回 nullptr。

    private:
        const MarkerGeometry &geometry_;  // matcher 内部存着模型的引用，干活时查表用。
    };

} // namespace mark