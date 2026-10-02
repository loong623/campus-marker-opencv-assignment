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
