// 黑盒！！！
// 读->验->写（三个函数）(读取 + 检查 + 转换) 具体实现暂时黑盒（时间不足以完全学会，以后遇到要改动再展开第二层（requireNode、readBool/readInt、lambda、错误设计）实现细节）
// 工程定位：负责把外部 YAML 配置文件，转换成程序内部可靠的 C++ 配置对象，并保证进入 Detector 前配置合法。
/* 报错表格配置板块契约（看懂就能理解报错输出、能验收、能调试）：
1.loadConfig 抛错（读的时候）：
->文件不存在/打不开
->必填字段缺失
->类型错误（开关不是 0/1 整数、mode 不是字符串）
->mode 不是 "skeleton"

2.validateConfig 抛错（验的时候）：
->schema_version ≠ 1、pixel_format ≠ BGR8、timestamp_unit ≠ "us"
->work_width/height ≤ 0、threshold 超出 0~255、max_hold_frames < 0
->6 个"未实现"开关被打开

3.writeEffectiveConfig 抛错：
->文件写不开

错误信息格式统一：file=哪个文件, field=哪个字段, reason=为啥错。
*/
/* 加字段后续可能增量：
1. struct 加成员；
2. loadConfig 加一段"读"；
3. validateConfig 加一条"验"；
4. writeEffectiveConfig 加一段"写"；
5. detector.yaml 加一行；
6. schema_version 升版（冻结要求"新增参数须版本化"）扩展点明确，不用碰老代码。
*/
/*  大体思路：
1. loadConfig(path)——读
打开 YAML 文件 → 逐个读 14 个字段。每个字段三步：看在不在（缺了抛错）、看类型对不对（开关必须是整数 0/1）、转成 C++ 类型（0/1→bool，"skeleton"→枚举）。读完再调 validateConfig 验一遍，返回填好的配置。
2. validateConfig(config)——验
不碰文件，只检查内存里的 struct。一条条过：版本号是不是 1、宽高是不是正数、阈值在不在 0~255、那几个"本板块不可启用"的开关有没有被打开（开了就报错"未实现"）。一条不过就抛 ConfigError。
3. writeEffectiveConfig(config, path)——写
把配置写回 YAML 文件。用途是导出"实际生效的配置"留档。要求：写出去再读回来，值必须一样（往返一致）。

两个小帮手：
throwConfigError：统一抛错，错误信息里带"哪个文件、哪个字段、为啥错"
readBool：读 0/1 开关，顺手校验只能是 0 或 1
*/
#include "config.hpp"

#include <opencv2/core.hpp>

#include "config_error.hpp"

// #include<iostream>  // debug临时加的

namespace mark
{

    namespace
    {   // 配置错误：文件=<path>，字段=<field>，原因=<reason>
        // 错误信息中英文对照：
        //   missing required field  → 缺少必填字段
        //   expected integer        → 应为整数
        //   expected integer 0/1    → 应为整数 0 或 1
        //   expected string         → 应为字符串
        //   unsupported mode        → 不支持的模式（只认 skeleton）
        //   unsupported version     → 不支持的版本（只认 1）
        //   unsupported format      → 不支持的格式（只认 BGR8）
        //   unsupported unit        → 不支持的单位（只认 us）
        //   must be positive        → 必须为正数（>0）
        //   must be non-negative    → 必须为非负数（≥0）
        //   range 0-255             → 超出范围 0~255
        //   cannot open file        → 无法打开文件
        //   cannot write file       → 无法写入文件
        //   not implemented         → 该功能在本板块未实现

        // 统一生成配置错误，保证错误信息包含文件路径和字段路径。
        [[noreturn]] void throwConfigError(
            const std::filesystem::path &file,
            const std::string &field,
            const std::string &message)
        {
            throw ConfigError(
                "Config error: file=" + file.string() + ", field=" + field + ", reason=" + message);
        }

        // 读取 YAML 中的开关字段，并转换成 C++ bool。
        bool readBool(
            const cv::FileNode &node,
            const std::filesystem::path &file,
            const std::string &field)
        {
            if (!node.isInt())
            {
                throwConfigError(file, field, "expected integer 0/1");
            }

            int value = 0;
            node >> value;

            if (value != 0 && value != 1)
            {
                throwConfigError(file, field, "expected integer 0/1");
            }

            return value == 1;
        }

        // 读取整数字段并检查基础类型。
        int readInt(
            const cv::FileNode &node,
            const std::filesystem::path &file,
            const std::string &field)
        {
            if (!node.isInt())
            {
                throwConfigError(file, field, "expected integer");
            }

            int value = 0;
            node >> value;

            return value;
        }

        // 读取浮点字段，并检查 YAML 基础类型。(避免 OpenCV YAML 类型坑)
        double readDouble(
            const cv::FileNode &node,
            const std::filesystem::path &file,
            const std::string &field)
        {
            if (!node.isReal() && !node.isInt())
            {
                throwConfigError(
                    file,
                    field,
                    "expected number");
            }

            double value = 0.0;

            node >> value;

            return value;
        }

    } // namespace

    // 从 YAML 文件读取配置，并转换成内部强类型配置。(把 YAML 里的文字配置，变成 C++ 能使用的结构。) 第一个函数！！！
    AppConfig loadConfig(const std::filesystem::path &path)
    {
        cv::FileStorage fs(
            path.string(),
            cv::FileStorage::READ);

        if (!fs.isOpened())
        {
            throw ConfigError(
                "Config error: file=" + path.string() + ", reason=cannot open file");
        }

        AppConfig config;

        // YAML 字段属于 DetectorConfig，因此统一从这里访问，避免应用层和检测器配置混淆。
        DetectorConfig &detector = config.detector_config;

        auto requireNode =
            [&](const std::string &field) -> cv::FileNode
        {
            cv::FileNode node = fs[field];

            if (node.empty())
            {
                throwConfigError(
                    path,
                    field,
                    "missing required field");
            }

            return node;
        };

        detector.schema_version =
            readInt(
                requireNode("schema_version"),
                path,
                "schema_version");

        cv::FileNode input =
            requireNode("input");

        if (input["pixel_format"].empty())
        {
            throwConfigError(
                path,
                "input.pixel_format",
                "missing required field");
        }

        input["pixel_format"] >> detector.input.pixel_format;

        if (input["timestamp_unit"].empty())
        {
            throwConfigError(
                path,
                "input.timestamp_unit",
                "missing required field");
        }

        input["timestamp_unit"] >> detector.input.timestamp_unit;

        cv::FileNode preprocess =
            requireNode("preprocess");

        detector.preprocess.work_width =
            readInt(
                preprocess["work_width"],
                path,
                "preprocess.work_width");

        detector.preprocess.work_height =
            readInt(
                preprocess["work_height"],
                path,
                "preprocess.work_height");

        detector.preprocess.threshold =
            readInt(
                preprocess["threshold"],
                path,
                "preprocess.threshold");

        // Block 2 几何观测配置。
        // 控制白色区域提取、多边形简化和面积过滤。
        cv::FileNode geometry =
            requireNode("geometry");

        detector.geometry_.white_threshold_ =
            readInt(
                geometry["white_threshold"],
                path,
                "geometry.white_threshold");

        detector.geometry_.approximation_epsilon_ =
            readDouble(
                geometry["approximation_epsilon"],
                path,
                "geometry.approximation_epsilon");

        detector.geometry_.min_area_ =
            readDouble(
                geometry["min_area"],
                path,
                "geometry.min_area");

        detector.geometry_.max_validation_residual_ =
            readDouble(
                geometry["max_validation_residual"],
                path,
                "geometry.max_validation_residual");

        detector.geometry_.min_area_ratio_ =
            readDouble(
                geometry["min_area_ratio"],
                path,
                "geometry.min_area_ratio");

        detector.geometry_.max_area_ratio_ =
            readDouble(
                geometry["max_area_ratio"],
                path,
                "geometry.max_area_ratio");

        // 几何假设搜索资源上限。
        detector.geometry_.max_hypothesis_count_ =
            static_cast<std::size_t>(
                readInt(
                    geometry["max_hypothesis_count"],
                    path,
                    "geometry.max_hypothesis_count"));

        cv::FileNode detector_node =
            requireNode("detector");

        // 先检查字段存在，再检查类型，避免 YAML 类型错误被直接转换掩盖。
        cv::FileNode mode_node =
            detector_node["mode"];

        if (mode_node.empty())
        {
            throwConfigError(
                path,
                "detector.mode",
                "missing required field");
        }

        if (!mode_node.isString())
        {
            throwConfigError(
                path,
                "detector.mode",
                "expected string");
        }

        std::string mode;
        mode_node >> mode;

        /* debug 工程经验：OpenCV cv::FileStorage 的 YAML 注释兼容性与标准 YAML 不完全一致，配置文件中避免使用行尾 # comment，否则可能被读取为字符串内容。
        这三行是"用数字代替眼睛"：

        1. mode.size() —— 字符串长度。"Skeleton" 该是 8，读出来 59，直接暴露多了东西

        2. for 循环打印每个字符的 ASCII 码 —— 比如 S=83、k=107、空格=32。肉眼看不见的字符（空格、换行、中文），在数字面前全现形

        3. mode=[...] —— 前后加方括号，原样打印。一眼看出开头结尾有没有偷藏空格

        4. 原理："看着对但跑不对"时，别用眼睛猜，用数字说话。 size=59 就是铁证——字符串里混进了注释。
        // debug 临时加的
        std::cout << "mode size = " << mode.size() << std::endl;

        std::cout << "mode chars:";
        for (unsigned char c : mode)
        {
            std::cout << " [" << static_cast<int>(c) << "]";
        }
        std::cout << std::endl;

        std::cout << "mode=[" << mode << "]" << std::endl;
        // debug end
        */

        if (mode == "Skeleton")
        {
            detector.mode = DetectorMode::Skeleton;
        }
        else
        {
            throwConfigError(
                path,
                "detector.mode",
                "unsupported mode");
        }

        cv::FileNode temporal =
            requireNode("temporal");

        detector.temporal.stabilization_enabled =
            readBool(
                temporal["stabilization_enabled"],
                path,
                "temporal.stabilization_enabled");

        detector.temporal.display_hold_enabled =
            readBool(
                temporal["display_hold_enabled"],
                path,
                "temporal.display_hold_enabled");

        detector.temporal.max_hold_frames =
            readInt(
                temporal["max_hold_frames"],
                path,
                "temporal.max_hold_frames");

        cv::FileNode output =
            requireNode("output");

        detector.output.show_window =
            readBool(
                output["show_window"],
                path,
                "output.show_window");

        detector.output.show_held_state =
            readBool(
                output["show_held_state"],
                path,
                "output.show_held_state");

        cv::FileNode debug =
            requireNode("debug");

        detector.debug.timing_enabled =
            readBool(
                debug["timing_enabled"],
                path,
                "debug.timing_enabled");

        detector.debug.draw_candidates =
            readBool(
                debug["draw_candidates"],
                path,
                "debug.draw_candidates");

        validateConfig(config);

        return config;
    }

    // 校验配置是否满足冻结字段要求和当前板块实现范围。（检查“配置虽然读进来了，但是能不能用”，即“格式正确，但参数错误”（合理性）；区别于loadConfig检查配置有没有。）
    void validateConfig(const AppConfig &config)
    {
        const DetectorConfig &detector =
            config.detector_config;

        if (detector.schema_version != 1)
        {
            throw ConfigError(
                "Config error: field=schema_version, reason=unsupported version");
        }

        if (detector.input.pixel_format != "BGR8")
        {
            throw ConfigError(
                "Config error: field=input.pixel_format, reason=unsupported format");
        }

        if (detector.input.timestamp_unit != "us")
        {
            throw ConfigError(
                "Config error: field=input.timestamp_unit, reason=unsupported unit");
        }

        if (detector.preprocess.work_width <= 0)
        {
            throw ConfigError(
                "Config error: field=preprocess.work_width, reason=must be positive");
        }

        if (detector.preprocess.work_height <= 0)
        {
            throw ConfigError(
                "Config error: field=preprocess.work_height, reason=must be positive");
        }

        if (detector.preprocess.threshold < 0 ||
            detector.preprocess.threshold > 255)
        {
            throw ConfigError(
                "Config error: field=preprocess.threshold, reason=range 0-255");
        }

        // geometry.white_threshold 与灰度阈值范围一致。
        if (detector.geometry_.white_threshold_ < 0 ||
            detector.geometry_.white_threshold_ > 255)
        {
            throw ConfigError(
                "Config error: field=geometry.white_threshold, reason=range 0-255");
        }

        // approxPolyDP 的 epsilon 不允许为负。
        if (detector.geometry_.approximation_epsilon_ < 0)
        {
            throw ConfigError(
                "Config error: field=geometry.approximation_epsilon, reason=must be non-negative");
        }

        // 面积过滤下限不允许为负。
        if (detector.geometry_.min_area_ < 0)
        {
            throw ConfigError(
                "Config error: field=geometry.min_area, reason=must be non-negative");
        }

        // 搜索上限必须大于 0。
        if (detector.geometry_.max_hypothesis_count_ == 0)
        {
            throw ConfigError(
                "Config error: field=geometry.max_hypothesis_count, reason=must be greater than zero");
        }

        // Step 7 残差验证阈值必须为正
        if (detector.geometry_.max_validation_residual_ <= 0)
        {
            throw ConfigError(
                "Config error: field=geometry.max_validation_residual, reason=must be positive");
        }

        // Step 7 面积比例范围检查
        if (detector.geometry_.min_area_ratio_ <= 0 ||
            detector.geometry_.max_area_ratio_ <= 0 ||
            detector.geometry_.min_area_ratio_ >=
                detector.geometry_.max_area_ratio_)
        {
            throw ConfigError(
                "Config error: field=geometry.area_ratio, reason=invalid range");
        }

        // temporal配置
        if (detector.temporal.max_hold_frames < 0)
        {
            throw ConfigError(
                "Config error: field=temporal.max_hold_frames, reason=must be non-negative");
        }

        if (detector.mode != DetectorMode::Skeleton)
        {
            throw ConfigError(
                "Config error: field=detector.mode, reason=unsupported mode");
        }

        // 当前板块没有实现这些功能，因此配置开启时必须拒绝，而不是静默忽略。
        if (detector.temporal.stabilization_enabled ||
            detector.temporal.display_hold_enabled ||
            detector.output.show_window ||
            detector.output.show_held_state ||
            detector.debug.timing_enabled ||
            detector.debug.draw_candidates)
        {
            throw ConfigError(
                "Config error: requested feature is not implemented in this block");
        }
    }

    // 将当前有效配置写出，并保证重新加载后配置值保持一致。(把程序最终实际使用的配置保存下来,用于debug，复现实验，记录比赛参数)
    /* 流程：
    用户yaml

    ↓

    loadConfig

    ↓

    补默认值/验证

    ↓

    当前实际配置

    ↓

    writeEffectiveConfig

    ↓

    保存
    */
    void writeEffectiveConfig(
        const AppConfig &config,
        const std::filesystem::path &path)
    {
        cv::FileStorage fs(
            path.string(),
            cv::FileStorage::WRITE);

        if (!fs.isOpened())
        {
            throw ConfigError(
                "Config error: file=" + path.string() + ", reason=cannot write file");
        }

        const DetectorConfig &detector =
            config.detector_config;

        fs << "schema_version"
           << detector.schema_version;

        fs << "input"
           << "{";

        fs << "pixel_format"
           << detector.input.pixel_format;

        fs << "timestamp_unit"
           << detector.input.timestamp_unit;

        fs << "}";

        fs << "preprocess"
           << "{";

        fs << "work_width"
           << detector.preprocess.work_width;

        fs << "work_height"
           << detector.preprocess.work_height;

        fs << "threshold"
           << detector.preprocess.threshold;

        fs << "}";

        fs << "geometry"
           << "{";

        fs << "white_threshold"
           << detector.geometry_.white_threshold_;

        fs << "approximation_epsilon"
           << detector.geometry_.approximation_epsilon_;

        fs << "min_area"
           << detector.geometry_.min_area_;

        fs << "max_hypothesis_count"
           << static_cast<int>(
                  detector.geometry_.max_hypothesis_count_);

        fs << "max_validation_residual"
           << detector.geometry_.max_validation_residual_;

        fs << "min_area_ratio"
           << detector.geometry_.min_area_ratio_;

        fs << "max_area_ratio"
           << detector.geometry_.max_area_ratio_;

        fs << "}";

        fs << "detector"
           << "{";

        fs << "mode"
           << "Skeleton";

        fs << "}";

        fs << "temporal"
           << "{";

        fs << "stabilization_enabled"
           << static_cast<int>(
                  detector.temporal.stabilization_enabled);

        fs << "display_hold_enabled"
           << static_cast<int>(
                  detector.temporal.display_hold_enabled);

        fs << "max_hold_frames"
           << detector.temporal.max_hold_frames;

        fs << "}";

        fs << "output"
           << "{";

        fs << "show_window"
           << static_cast<int>(
                  detector.output.show_window);

        fs << "show_held_state"
           << static_cast<int>(
                  detector.output.show_held_state);

        fs << "}";

        fs << "debug"
           << "{";

        fs << "timing_enabled"
           << static_cast<int>(
                  detector.debug.timing_enabled);

        fs << "draw_candidates"
           << static_cast<int>(
                  detector.debug.draw_candidates);

        fs << "}";

        fs.release();
    }

} // namespace mark

// 老版本留档(骨架)
/*
// config.hpp 的具体实现(骨架,还没有真正解析 YAML 字段)
#include "config.hpp"

#include <opencv2/core.hpp>

#include "config_error.hpp"

namespace mark
{
    // 根据路径读取配置文件，返回 AppConfig
    AppConfig loadConfig(const std::filesystem::path &path)
    {
        // 检查文件是否存在
        if (!std::filesystem::exists(path))
        {
            throw ConfigError(
                "Config file does not exist: " + path.string());
        }

        // 使用 OpenCV 的 FileStorage 读取 YAML 配置文件（打开 YAML）cv::FileStorage::READ表示打开模式：读取
        cv::FileStorage fs(
            path.string(),
            cv::FileStorage::READ);

        // 判断是否打开成功
        if (!fs.isOpened())
        {
            throw ConfigError(
                "Cannot open config file: " + path.string());
        }

        // 创建配置对象（空配置现在）
        AppConfig config;

        // TODO:
        // 当前只建立配置加载骨架。
        // Current implementation only provides config loading skeleton.
        //
        // TODO:
        // 等 §5.5 YAML字段契约确定后，
        // 在此解析 schema_version、input、preprocess 等字段。
        //
        // Parse schema_version, input, preprocess and other fields
        // after §5.5 YAML field contract is implemented.

        // 释放文件（关闭 YAML 文件）
        fs.release();

        // 验证配置是否合法（目前没有字段可验证）
        validateConfig(config);

        // 返回配置对象
        return config;
    }

    // 验证配置是否合法（目前没有字段可验证）
    void validateConfig(const AppConfig &config)
    {
        (void)config;

        // TODO:
        // 当前 DetectorConfig 仍为空结构，无实际字段可校验。
        // DetectorConfig has no frozen fields yet, so no validation is implemented.
        //
        // 等 §5.5 字段冻结后补充合法性检查。
        // Add validation after §5.5 fields are frozen.
    }

    // 把最终使用的配置写出去（保存“程序实际使用的最终配置”，方便复现、调试和排查问题）
    void writeEffectiveConfig(
        const AppConfig &config,
        const std::filesystem::path &path)    // std::filesystem::path ：只写文件名（在当前程序运行目录下创建）；写到指定目录；绝对路径
    {
        (void)config;

        cv::FileStorage fs(
            path.string(),
            cv::FileStorage::WRITE);         // 写文件

        if (!fs.isOpened())
        {
            throw ConfigError(
                "Cannot write config file: " + path.string());
        }

        // TODO:
        // 当前没有冻结字段可导出。
        // No frozen config fields are available for export yet.
        //
        // 等 §5.5 字段实现后写入有效配置。
        // Write effective config after §5.5 implementation.

        fs.release();
    }

} // namespace mark


用户配置文件
(detector.yaml)

        ↓

config.cpp
(读取 + 检查 + 转换)

        ↓

AppConfig

        ↓

DetectorConfig

        ↓

Detector运行

*/