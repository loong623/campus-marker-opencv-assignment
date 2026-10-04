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