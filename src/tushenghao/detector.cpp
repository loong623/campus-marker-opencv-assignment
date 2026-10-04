#include "detector.hpp"
#include "app_config.hpp"
#include "config.hpp"
#include "marker_geometry.hpp"
#include "preprocess.hpp"
#include "geometry_observation.hpp"
#include "geometry_matcher.hpp"
#include "geometry_validation.hpp"
namespace mark
{

    struct Detector::Impl
    {
        DetectorConfig config;

        // Block 2 MARK 几何模型
        MarkerGeometry marker_geometry;  // 注意名字是 marker_geometry，后面构造用

        // TODO:
        // 后续板块补充检测状态，例如时序缓存、跟踪状态等。
    };

    Detector::Detector(DetectorConfig config)
        : impl_(std::make_unique<Impl>())
    {
        // 防御式设计（工程接口可靠）二次校验
        // 构造时再次校验配置，避免绕过配置文件(YAML)直接传入非法配置。（Detector 自己也保护接口边界，即使绕过 YAML 直接构造，也必须通过验证。）
        AppConfig app_config;                  // 包装一层 AppConfig(防止传入类型接口不匹配)

        app_config.detector_config = config;

        validateConfig(app_config);              // 验证函数站在应用配置总入口检查

        impl_->config = std::move(config);       // 检查是否合法后，保存到 Detector 内部

        // 加载 Block 2 MARK 几何模型(构造函数加载 load geometry)
        impl_->marker_geometry =
            loadMarkerGeometry(
                impl_->config.marker_geometry_path_);
    }

    /*第一版错误：构造函数里直接调用 validateConfig(config) 报错：
    Detector::Detector(DetectorConfig config)
        : impl_(std::make_unique<Impl>())
    {
        // 构造时再次校验配置，避免绕过配置文件直接传入非法配置。
        validateConfig(config);

        impl_->config = std::move(config);          // config报错：void validateConfig(const AppConfig&);定义矛盾
    }
    */

    Detector::~Detector() = default;

    FrameResult Detector::process(const FrameInput &frame)
    {
        FrameResult result;

        // Block 1：预处理，得到工作图和帧上下文。
        PreparedFrame prepared =
            preprocess(frame, impl_->config.preprocess);

        // Step 5：提取白色连通域。
        auto components =
            extractWhiteComponents(
                prepared,
                impl_->config.geometry_);

        // 保存 Block 2 已提取的白色组件。
        // Block 3 通过 GeometryHypothesis 的 component_id_
        // 回查当前帧真实观测，不重新做白块分割。
        prepared.components_ = components;  // 把 extractWhiteComponents() 刚算出来的白块列表，存进 prepared.components_(顺手配套修改)

        // Step 5：观测 L/M/S 形状。
        auto observations =
            observeShapes(
                components,
                impl_->config.geometry_);

        // Step 6：生成几何假设。
        GeometryBatch batch =
            generateGeometryHypotheses(
                observations,
                impl_->marker_geometry,
                impl_->config.geometry_);

        // Step 7：独立验证假设。
        batch =
            validateGeometryBatch(
                batch,
                impl_->marker_geometry,
                components,
                impl_->config.geometry_);

        // 审计：记录本帧各阶段数量，供离线分析。
        batch.diagnostics_.push_back(
            "geometry_audit: components=" +
            std::to_string(components.size()) +
            ", observations=" +
            std::to_string(observations.size()) +
            ", hypotheses=" +
            std::to_string(batch.hypotheses_.size()) +
            ", resource_truncated=" +
            (batch.resource_truncated_ ? "true" : "false"));

        // Block 3 未实现，保持 NOT_READY。
        result.status = Status::NOT_READY;

        return result;
    }

    void Detector::reset(ResetReason reason) noexcept
    {
        (void)reason;

        // 当前阶段没有内部状态，后续加入时序状态后在此清理。
    }

    const DetectorConfig &Detector::config() const noexcept
    {
        return impl_->config;
    }

} // namespace mark

/*
## detector.cpp 更新（2026-10-03 1:30）

**改了啥：**
1. 构造函数加 `validateConfig(config)` 二次校验 —— 构造时二次校验
2. `process` 简化：直接返回空 FrameResult，不再回填帧信息 —— 纯骨架
3. `Impl` 加 TODO，为后续时序/跟踪状态留位置 —— 后续板块补时序/跟踪状态

**为啥：**
- §5.6 要求：绕过 loadConfig 直接 `new Detector` 时，非法配置也必须被拒
- 6 小时前字段没冻结，写不了校验；现在补上
- process 真正实现是板块2 的事，现在不瞎写

// detector.hpp 的实现部分(暂时骨架，等待加入具体检测算法)
#include "detector.hpp"   // 头文件自己应该保证“自洽”

namespace mark
{
    //PImpl 的另一半（内部细节）（外部补充私有定义）
    struct Detector::Impl
    {
        DetectorConfig config;
    };

    //构造函数（初始化 Impl（智能管理，类似于new，但不要delete），保存配置（把传入的config副本直接放入内部，不复制了））
    Detector::Detector(DetectorConfig config)
        : impl_(std::make_unique<Impl>())
    {
        impl_->config = std::move(config);
    }

    //析构函数（使用默认析构，unique_ptr智能指针自动释放 Impl）
    Detector::~Detector() = default;

    // 创建输出
    FrameResult Detector::process(const FrameInput &frame)
    {
        FrameResult result;

        result.frame_id = frame.frame_id;
        result.timestamp_us = frame.timestamp_us;
        result.status = Status::NOT_READY;      // 当前检测还没实现

        return result;
    }

    // 重置状态（当前没有状态，暂时不做任何操作）（以后tracking状态，历史帧，缓存等）
    void Detector::reset(ResetReason reason) noexcept
    {
        (void)reason;          // 避免未使用参数警告（unused parameter warning）（void）
    }

    // 获取当前配置（只读访问,返回当前配置）
    const DetectorConfig &Detector::config() const noexcept
    {
        return impl_->config;
    }

} // namespace mark
*/