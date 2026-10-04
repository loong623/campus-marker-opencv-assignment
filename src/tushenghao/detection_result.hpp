// 几何模型真正接入检测流程前的准备层(Geometry primitive integration)
/*DetectionResult：死的，就是个结构体，装数据。检测器填好，扔出去就不管了。(包裹)
Detector
    |
    v
DetectionResult
*/
// 几何原语接入阶段的数据结构（detector 和 geometry中间数据接口）
/* 目前结构：
Detector
    |
    v
DetectionResult
    |
    v
GeometryMatcher
    |
    v
MatchedMarker

DetectionResult
        |
        v
GeometryMatcher
        |
        v
GeometryPolygon
*/
#pragma once
// 已废弃：方向错误，仅保留作历史参考，勿在此基础上继续开发。
// 本文件假设 ID 已知（ID 查表），但实战中 ID 正是几何推理要求出的结果，
// 不能由检测器直接提供。正确路线见 geometry_types.hpp。
// 待正式链路跑通后，将通过 cleanup commit 删除。


#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace mark
{

    // 单个 MARK 检测结果。
    // 表示 detector 输出给后续几何匹配模块的数据。
    struct DetectionResult
    {
        // 检测到的 MARK ID，例如 L0 / M1。
        std::string id;

        // 图像坐标系中的关键点。
        // 顺序需要与几何模型中的 vertices 对应。
        std::vector<cv::Point2f> image_points;

        // 检测区域中心点。
        cv::Point2f center;

        // 当前检测可信度。
        double confidence{0.0};
    };

} // namespace mark