// Detector 自己需要的配置结构（只包含检测算法配置）14 个 YAML 字段（冻结文档，配置文件语言） → C++ 成员（映射，程序语言）
// Detector 模块的配置结构体定义（定义 外部怎么配置 Detector）（配置怎么传递）（公共接口层组分）
#pragma once

#include <string>
#include <cstddef>

namespace mark
{
    // 把 YAML 里的配置，映射成 C++ 结构，让 Detector 使用

    // 检测器
    // YAML detector.mode uses "skeleton"; enum limits valid states.
    // YAML detector.mode 使用字符串 "skeleton"，enum 用于限制合法状态空间。
    enum class DetectorMode
    {
        Skeleton
    };          // 现在是骨架模式（啥也不干，返回 NOT_READY）

    // 输入相关配置
    // Keep C++ structure aligned with YAML hierarchy for field-path validation.
    // 保持 C++ 结构与 YAML 层级一致，便于错误字段路径对应。
    struct InputConfig
    {
        std::string pixel_format = "BGR8";        // 图像格式（OpenCV 默认彩色格式）
        std::string timestamp_unit = "us";        //时间戳单位是微秒
    };

    // 预处理参数（原图->resize->thershold->findCountours）
    struct PreprocessConfig
    {
        int work_width = 960;
        int work_height = 720;             // 图像统一缩到这个尺寸再算（快）
        int threshold = 200;               // 二值化阈值，灰度超 200 变白——用来找白色角标
    };

    // 几何观察相关配置。
    // Block2 使用这些参数从白色区域中提取几何信息。
    // 具体阈值由后续合成数据测量确定。// TODO: 以下参数为占位值，待合成数据测量后调优（校内赛前）。
    struct GeometryConfig
    {
        // 白色区域二值化阈值。
        // 为什么需要：
        // extractWhiteComponents 需要把白色 MARK 区域从背景中分离出来。
        // 当前值只是占位，后续通过数据测量调整。
        int white_threshold_ = 200;

        // approxPolyDP 多边形简化精度。（简化误差）
        // 为什么需要：
        // 原始 contour 点数量可能过多，需要压缩成几何顶点。
        // 当前值只是占位。
        double approximation_epsilon_ = 2.0;

        // 白色组件面积过滤下限。
        // 为什么需要：
        // 去除噪声产生的小连通区域。
        // 当前值只是占位。
        double min_area_ = 0.0;

        // 几何假设搜索最大数量。
        // 防止组合爆炸导致资源不可控。
        std::size_t max_hypothesis_count_ = 1000;

        // Block2 Step 7 几何验证最大允许残差。
        // 用于比较模型投影轮廓与实际观测轮廓。
        double max_validation_residual_ = 5.0;

        // Step 7 面积比例验证范围。
        // ratio = 投影面积 / 观测面积。
        double min_area_ratio_ = 0.5;

        double max_area_ratio_ = 2.0;
    };

    // 时序相关
    struct TemporalConfig
    {
        // YAML 0/1 -> C++ bool: configuration switch becomes semantic state.
        // YAML 0/1 → C++ bool：配置开关直接表达启用状态。
        bool stabilization_enabled = false;          // 是否开启稳定

        bool display_hold_enabled = false;           // 是否允许显示保持状态                     // 开关都是 0（关）

        int max_hold_frames = 5;                     // 最多保持5帧
    };

    // 输出/显示相关
    struct OutputConfig
    {
        bool show_window = false;            //控制cv::imshow()是否打开

        bool show_held_state = false;        //历史保持状态是否显示                          // 都是 0（不弹窗）
    };         // 它属于 App 行为。但这里先放在 DetectorConfig 里，是冻结结构决定的。

    // 调试开关
    struct DebugConfig
    {
        bool timing_enabled = false;         // 打开处理耗时统计

        bool draw_candidates = false;        // 显示亮斑候选，轮廓，角点方便调试           // 都是 0（关，板块5才开）
    };

    // DetectorConfig 总结构
    struct DetectorConfig
    {
        int schema_version = 1;           // 配置文件版本号，以后加字段就升版

        InputConfig input;

        PreprocessConfig preprocess;

        GeometryConfig geometry_;

        DetectorMode mode = DetectorMode::Skeleton;

        TemporalConfig temporal;

        OutputConfig output;

        DebugConfig debug;
    };              // 和 YAML 配置层级一致！参数  给定默认值再覆盖（如果 YAML 没写，使用默认值）

} // namespace mark

/*
读取 app.yaml

        ↓

AppConfig

        ↓

DetectorConfig

        ↓

创建 Detector（使用配置进行检测）
*/