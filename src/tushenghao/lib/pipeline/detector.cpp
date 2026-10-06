#include "mark/detector.hpp"
#include "config/app_config.hpp"
#include "config/config.hpp"
#include "core/marker_geometry.hpp"
#include "pipeline/decode_stage.hpp"
#include <optional>
#include <utility>
namespace mark
{

    struct Detector::Impl
    {
        DetectorConfig config;

        // Block 2 MARK 几何模型
        MarkerGeometry marker_geometry; // 注意名字是 marker_geometry，后面构造用

        // 仅保存输入序列校验所需的元数据；不缓存图像、角点或方向。
        std::optional<std::pair<uint64_t, int64_t>> last_input;
    };

    Detector::Detector(DetectorConfig config)
        : impl_(std::make_unique<Impl>())
    {
        // 防御式设计（工程接口可靠）二次校验
        // 构造时再次校验配置，避免绕过配置文件(YAML)直接传入非法配置。（Detector 自己也保护接口边界，即使绕过 YAML 直接构造，也必须通过验证。）
        AppConfig app_config; // 包装一层 AppConfig(防止传入类型接口不匹配)

        app_config.detector_config = config;

        validateConfig(app_config); // 验证函数站在应用配置总入口检查

        impl_->config = std::move(config); // 检查是否合法后，保存到 Detector 内部

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

    // 输入当前帧，校验公共生命周期，输出 metadata 和明确状态。
    // 旧代码将 decode 当完整流程并造透传 track；稳定层未实现必须 NOT_READY。
    FrameResult Detector::process(const FrameInput &frame)
    {
        FrameResult result{};
        result.frame_id = frame.frame_id;
        result.timestamp_us = frame.timestamp_us;
        result.status = Status::INVALID_INPUT;
        if (frame.image.empty() || frame.image.dims != 2 || frame.image.type() != CV_8UC3 ||
            frame.timestamp_us < 0 || frame.time_source != TimestampSource::Unknown)
        {
            result.diagnostics.push_back("INPUT_FORMAT: 需要BGR8、非负微秒时间及合法时间来源枚举");
            return result;
        }
        if (impl_->last_input &&
            (frame.frame_id <= impl_->last_input->first || frame.timestamp_us < impl_->last_input->second))
        {
            result.diagnostics.push_back("INVALID_SEQUENCE: 帧号须递增，时间不可倒退；更换输入请reset");
            return result;
        }
        impl_->last_input = std::make_pair(frame.frame_id, frame.timestamp_us);
        // 与audit复用唯一阶段实现；缺预算的内部阶段显式NOT_READY。
        // 当前公开流程仍不发布阶段Detection/透传track，等待真实稳定层完成。
        auto stage = runDecodePipeline(frame, impl_->config, impl_->marker_geometry);
        result.diagnostics = std::move(stage.diagnostics);
        result.status = Status::NOT_READY;
        result.diagnostics.push_back("STABILIZE_NOT_READY: 稳定层未实现；decode请使用阶段审计入口");
        return result;
    }

    void Detector::reset(ResetReason reason) noexcept
    {
        (void)reason;

        // reset 清除输入生命周期，新的序列可从任意合法帧号重新开始。
        impl_->last_input.reset();
    }

    const DetectorConfig &Detector::config() const noexcept
    {
        return impl_->config;
    }

} // namespace mark
