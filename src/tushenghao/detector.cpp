#include "detector.hpp"
#include "app_config.hpp"
#include "config.hpp"

namespace mark
{

    struct Detector::Impl
    {
        DetectorConfig config;

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
        (void)frame;            // 防止unused parameter 'frame'

        // 当前阶段只建立接口骨架，实际检测流程由后续板块实现。
        FrameResult result;

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