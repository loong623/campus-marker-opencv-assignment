// 预处理过程——输入原始帧，输出处理好的 PreparedFrame
#include "preprocess.hpp"

#include <stdexcept>

#include <opencv2/imgproc.hpp>

namespace mark
{

    PreparedFrame preprocess(
        const FrameInput &frame,
        const PreprocessConfig &config)
    {
        if (frame.image.empty())
        {
            throw std::invalid_argument(
                "preprocess: input image is empty");  // 输入图像为空
        }

        if (config.work_width <= 0 ||
            config.work_height <= 0)
        {
            throw std::invalid_argument(
                "preprocess: invalid working size");    // 工作尺寸无效
        }

        PreparedFrame prepared;

        /*
         * 1. 保存帧上下文
         */
        prepared.frame_id_ = frame.frame_id;
        prepared.timestamp_us_ = frame.timestamp_us;

        /*
         * 2. resize
         *
         * 原图:
         *   original_width  x original_height
         *
         * 工作图:
         *   work_width x work_height
         */
        cv::Mat resized;

        cv::resize(
            frame.image,
            resized,
            cv::Size(
                config.work_width,
                config.work_height));

        prepared.scale_x_ =
            static_cast<double>(config.work_width) /
            static_cast<double>(frame.image.cols);

        prepared.scale_y_ =
            static_cast<double>(config.work_height) /
            static_cast<double>(frame.image.rows);

        /* 违反了“只做预处理，不做后续处理”的原则，暂时注释掉。
         * 3. threshold
         *
         * 输出二值工作图。
         * Block2 后续基于该结果提取白色区域。
         
        cv::Mat gray;

        if (resized.channels() == 3)
        {
            cv::cvtColor(
                resized,
                gray,
                cv::COLOR_BGR2GRAY);
        }
        else
        {
            gray = resized;
        }

        cv::threshold(
            gray,
            prepared.image_,
            config.threshold,
            255,
            cv::THRESH_BINARY);
        */

        /*
         * 3. 输出 BGR 工作图
         *
         * 冻结契约：PreparedFrame::image_ 必须是 3 通道 BGR。
         * 灰度化/二值化由 Block 2 Step 5 (geometry_observation) 负责，
         * 这里不做，否则 Step 5 的 cvtColor(BGR2GRAY) 会因输入已是单通道而崩。
         */
        prepared.image_ = resized;

        return prepared;
    }

} // namespace mark