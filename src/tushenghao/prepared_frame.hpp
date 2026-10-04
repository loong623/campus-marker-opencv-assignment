/*实现范围：预处理后的当前帧数据
FrameInput
    |
    v
preprocess()
    |
    +-- resize
    |
    +-- threshold
    |
    +-- coordinate mapping
    |
    +-- frame context
    |
    v
PreparedFrame
*/
#pragma once

#include <cstdint>
#include <vector>

#include "geometry_types.hpp"

#include <opencv2/core.hpp>

namespace mark
{

    // Detector preprocess 阶段输出。
    // 保存工作图以及从原图到工作图的显式坐标映射。
    // Block2 后续只消费该结构，不负责恢复映射关系。
    struct PreparedFrame
    {
        // preprocess 后的工作图。
        // 当前阶段输出 threshold 后的二值工作图。
        cv::Mat image_;

        // 原图坐标 -> 工作图坐标的缩放比例。
        // working_x = original_x * scale_x_
        // working_y = original_y * scale_y_
        double scale_x_{1.0};

        double scale_y_{1.0};

        /**
         * @brief 当前帧白色连通区域
         *
         * Block 2 分割阶段生成。
         *
         * Block 3 根据 GeometryHypothesis 中的 component_id_
         * 直接回查这里的真实观测轮廓。
         * 加 components_ 以补这条链：component_id_ → frame.components_[id] → contour_
         *
         * 不重新分割，避免产生第二套观测结果。
         */
        std::vector<WhiteComponent> components_;

        // 帧上下文。
        // 用于后续阶段 audit 和结果关联。
        uint64_t frame_id_{0};

        int64_t timestamp_us_{0};
    };

} // namespace mark

/*  初版：
// C++ 类型依赖补充（block2）
#pragma once

#include <opencv2/core.hpp>

namespace mark
{

    // Detector preprocess 阶段输出。
    // Block2 只消费处理后的图像，不关心前处理过程。
    // image_ 固定为 BGR 工作图（CV_8UC3）。
    // Block 2 会自行转换为灰度图进行白色区域提取。
    struct PreparedFrame
    {
        // 当前工作图像。
        // geometry observation 使用该图像进行 threshold 和 contour 提取。
        cv::Mat image_;
    };

} // namespace mark
 */