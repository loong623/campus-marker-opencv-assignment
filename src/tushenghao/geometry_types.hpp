#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

namespace mark
{

    // 转折类型：显式区分简化多边形顶点是凸转折还是凹转折。
    enum class TurnType
    {
        CONVEX,
        CONCAVE
    };

    // 单个多边形转折特征：把顶点位置和凸凹语义绑定，避免使用含义不明的整数编码。
    struct TurnFeature
    {
        // 对应 simplified_polygon_ 中的顶点下标，用于把转折语义绑定到具体顶点。
        std::size_t vertex_index_{0};

        // 当前顶点的转折类型，取值仅为 CONVEX 或 CONCAVE。
        TurnType type_{TurnType::CONVEX};
    };

    // Block 2 早期完整性状态：这里只表达明确不完整或仍待后续验证。
    enum class GeometryCompleteness
    {
        CLEARLY_INCOMPLETE,
        PENDING_VALIDATION
    };

    // 单个白色连通区域：保存图像分割得到的原始白片观测。
    struct WhiteComponent
    {
        // 帧内唯一组件编号，用于后续 assignment 准确引用同一白片。
        std::size_t component_id_{0};

        // 原始轮廓，用于保留未经多边形简化的观测证据。
        std::vector<cv::Point> contour_;

        // 原始轮廓面积，用于后续相对面积和尺度检查。
        double area_{0.0};

        // 白片在工作图中的外接范围，用于空间关系和边界检查。
        cv::Rect bounding_box_;

        // 是否接触工作图边界，用于识别可能被裁切的不完整结构。
        bool touches_border_{false};
    };

    // 白片的结构化几何观测：保存简化后的形状证据及其不确定性。
    struct ShapeObservation
    {
        // 简化后的有序多边形，用于后续 L/M/S 结构分析。
        std::vector<cv::Point2f> simplified_polygon_;

        // 各关键顶点的凸凹转折信息，显式记录顶点下标和转折类型。
        std::vector<TurnFeature> turns_;

        // 多边形简化相对原始轮廓的误差，用于判断结构观测是否仍可信。
        double simplification_error_{0.0};

        // 当前观测支持的候选类别，字符串只允许 "L"、"M"、"S"。
        std::vector<std::string> supported_classes_;
    };

    // 模型片段与观测白片的对应关系：记录一个模型组件由哪一个帧内白片解释。
    struct ComponentAssignment
    {
        // 模型片段编号，例如 "L0"、"L2"、"L3"、"M1"、"S1a" 或 "S1b"。
        std::string model_part_id_;

        // 对应 WhiteComponent::component_id_，用于引用当前帧中的实际观测白片。
        std::size_t component_id_{0};
    };

    // 几何假设的早期完整性采用 §4.7 状态，而不是用连续分数冒充最终完整性判断。
    struct GeometryHypothesis
    {
        // 当前假设采用的全部模型片段到观测白片对应，用于保留完整解释关系。
        std::vector<ComponentAssignment> assignments_;

        // 模型坐标到工作图坐标的 2x3、CV_64F 仿射矩阵，方向固定为 model -> working image。
        // 它只表示二维仿射对应，不是相机位姿。
        cv::Mat affine_transform_;

        // 未参与拟合的独立结构验证残差，用于判断该对应是否真正得到额外观测支持。
        double validation_residual_{0.0};

        // Block 2 只区分“明确不完整”和“待验证”，最终完整性留给 Block 3。
        GeometryCompleteness completeness_{
            GeometryCompleteness::PENDING_VALIDATION};

        // 记录该假设被保留或拒绝判断所依据的几何证据，便于后续 audit 定位原因。
        std::vector<std::string> evidence_;
    };

    // 一帧的几何推理结果：汇总全部合格假设以及搜索过程状态。
    struct GeometryBatch
    {
        // 保存当前帧全部合格几何假设，因为 Block 2 不强行选择唯一解释。
        std::vector<GeometryHypothesis> hypotheses_;

        // 保存本帧几何阶段的诊断信息，用于定位提取、组合或验证失败原因。
        std::vector<std::string> diagnostics_;

        // 表示搜索是否因资源上限而被截断，因为截断后的结果不能宣称搜索完整。
        bool resource_truncated_{false};
    };

} // namespace mark