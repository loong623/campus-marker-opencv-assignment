/*decode_audit.cpp 是离线审计工具：拿视频跑一遍，打印每帧结果。换视频就换个路径参数，换输入源（比如改成读图片序列）改几行就行。（block 3 step 7审计使用验证）
Block 5 的豪华版debug是运行时诊断系统：在线记录、序列化、结构化数据，给系统运行时用的。
两者不冲突：
平时调代码、回归测试 → 用 decode_audit（轻便）
系统跑起来要分析性能、存日志 → 用 Block 5 的（正式）
*/
// 换视频不用改代码，直接换视频路径（命令行参数，目前是相对路径，有cwd依赖，以后有需要再修），只有换输入源类型才要动（比如从视频文件改成摄像头实时流，那要改 cv::VideoCapture 的打开方式）
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#include <opencv2/videoio.hpp>

#include "config.hpp"
#include "detector.hpp"
#include "detector_types.hpp"

namespace
{

    /**
     * @brief 解析命令行中的帧号列表。
     *
     * 输入：
     *
     * "10,25,30"
     *
     * 输出：
     *
     * {10,25,30}
     *
     * 不负责检查视频范围，
     * 后续读取视频时处理。
     */
    std::vector<int> parseFrameList(
        const std::string &text)
    {
        std::vector<int> frames;

        std::stringstream ss(text);

        std::string item;

        while (std::getline(
            ss,
            item,
            ','))
        {
            if (!item.empty())
            {
                frames.push_back(
                    std::stoi(item));
            }
        }

        return frames;
    }

    /**
     * @brief 判断当前帧是否需要处理。
     *
     * 没有指定列表：
     *
     * 全部处理。
     *
     * 指定列表：
     *
     * 只处理指定帧。
     */
    bool shouldProcessFrame(
        int frame_id,
        const std::vector<int> &selected_frames)
    {
        if (selected_frames.empty())
        {
            return true;
        }

        for (int id :
             selected_frames)
        {
            if (id == frame_id)
            {
                return true;
            }
        }

        return false;
    }

    void printDiagnostics(
        const std::vector<std::string> &diagnostics)
    {
        if (diagnostics.empty())
        {
            std::cout
                << "(none)";

            return;
        }

        for (const auto &item :
             diagnostics)
        {
            std::cout
                << item
                << " ";
        }
    }

} // namespace

int main(
    int argc,
    char **argv)
{
    if (argc < 2)
    {
        std::cerr
            << "用法: decode_audit <视频路径> [帧号列表]\n";

        return 1;
    }

    const std::string video_path =
        argv[1];

    std::vector<int> selected_frames;

    if (argc >= 3)
    {
        selected_frames =
            parseFrameList(
                argv[2]);
    }

    /*
     * Step 7:
     *
     * 加载正式配置。
     *
     * 工具不自己创建参数，
     * 保证回归环境和 Detector 一致。
     */
    /*  debug换绝对路径
    mark::AppConfig app_config =
        mark::loadConfig(
            "src/tushenghao/config/detector.yaml");  // Workaround(相对路径,要求站在仓库根目录)
    */

    mark::AppConfig app_config =
        mark::loadConfig(
            ([]()
             {
    namespace fs = std::filesystem;
    // __FILE__ = .../src/tushenghao/tools/decode_audit.cpp
    // parent_path x2 = .../src/tushenghao
    fs::path base = fs::path(__FILE__).parent_path().parent_path();
    return (base / "config" / "detector.yaml").string(); })());

    mark::Detector detector(
        app_config.detector_config);

    cv::VideoCapture capture(
        video_path);

    if (!capture.isOpened())
    {
        std::cerr
            << "无法打开视频: "
            << video_path
            << "\n";

        return 1;
    }

    int frame_id = 0;

    int total_frames = 0;

    int valid_frames = 0;

    int rejected_frames = 0;

    while (true)
    {
        cv::Mat frame;

        if (!capture.read(frame))
        {
            break;
        }

        if (!shouldProcessFrame(
                frame_id,
                selected_frames))
        {
            ++frame_id;

            continue;
        }

        ++total_frames;

        /*
         * 复用 Detector pipeline。
         *
         * decode_audit 不参与：
         *
         * - 分割
         * - 匹配
         * - 几何推理
         *
         * 只记录结果。
         */
        mark::FrameInput input;

        input.image = frame;

        input.frame_id = static_cast<uint64_t>(frame_id);

        mark::FrameResult result =
            detector.process(
                input);

        if (!result.detections.empty())
        {
            ++valid_frames;
        }
        else
        {
            ++rejected_frames;
        }

        std::cout
            << "帧号: "
            << frame_id
            << " | detections: "
            << result.detections.size()
            << " | status: "
            << static_cast<int>(
                   result.status)
            << " | diagnostics: ";

        printDiagnostics(
            result.diagnostics);

        std::cout
            << "\n";

        ++frame_id;
    }

    std::cout
        << "\n===== Audit Summary =====\n";

    std::cout
        << "总处理帧数: "
        << total_frames
        << "\n";

    std::cout
        << "有效帧: "
        << valid_frames
        << "\n";

    std::cout
        << "拒绝帧: "
        << rejected_frames
        << "\n";

    return 0;
}