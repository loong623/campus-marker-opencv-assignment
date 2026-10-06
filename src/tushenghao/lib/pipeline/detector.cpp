#include "mark/detector.hpp"
#include "config/app_config.hpp"
#include "config/config.hpp"
#include "core/marker_geometry.hpp"
#include "pipeline/stabilize_stage.hpp"
#include "pipeline/frame_sequence.hpp"
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
        std::optional<FrameStamp> last_input;
        std::optional<cv::Size> original_size;
        std::unique_ptr<TemporalStabilizer> temporal;
        std::unique_ptr<DisplayHistory> display;
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

    Detector::~Detector() = default;

    // D16旧非法分支留下缓存；公共入口统一全清，缺G-B不能用关闭模式绕过。
    FrameResult Detector::process(const FrameInput &frame)
    {
        FrameResult result{};result.frame_id=frame.frame_id;result.timestamp_us=frame.timestamp_us;
        result.status=Status::INVALID_INPUT;
        FrameStamp stamp{frame.frame_id,frame.timestamp_us,frame.time_source};std::string reason;
        if(frame.image.empty() || frame.image.dims!=2 || frame.image.type()!=CV_8UC3 ||
           !validateFrameStamp(stamp,impl_->last_input,reason)) {
            reset(ResetReason::InvalidSequence);
            result.diagnostics.push_back(reason.empty()?"INPUT_FORMAT":reason);return result;
        }
        const bool size_changed=impl_->original_size && *impl_->original_size!=frame.image.size();
        if(size_changed) reset(ResetReason::InputChanged);
        const auto& c=impl_->config;
        if(!c.assignment_completion_ || !c.corner_.observation_budget_ ||
           !c.temporal.correspondence_uncertainty_px || !c.temporal.max_smoothing_deviation_px) {
            reset(ResetReason::External);impl_->last_input=stamp;impl_->original_size=frame.image.size();result.status=Status::NOT_READY;
            result.diagnostics.push_back("PIPELINE_BUDGET_MISSING");return result;
        }
        if(!impl_->temporal) impl_->temporal=std::make_unique<TemporalStabilizer>(c.temporal);
        if(!impl_->display) impl_->display=std::make_unique<DisplayHistory>(DisplayHistoryConfig{c.temporal.display_hold_enabled,c.temporal.max_hold_frames});
        auto decoded=runDecodePipeline(frame,c,impl_->marker_geometry);
        result=finalizeDecodedFrame(decoded,stamp,frame.image.size(),*impl_->temporal,*impl_->display);
        if(size_changed) result.diagnostics.push_back("INPUT_CHANGED");
        if(result.status==Status::INVALID_INPUT || result.status==Status::NOT_READY) reset(ResetReason::InvalidSequence);
        else {impl_->last_input=stamp;impl_->original_size=frame.image.size();}
        return result;
    }

    // 三历史与序列/尺寸同时清，重复reset安全，不重新加载模型和配置。
    void Detector::reset(ResetReason reason) noexcept
    {
        impl_->last_input.reset();impl_->original_size.reset();
        if(impl_->temporal) impl_->temporal->reset(reason);
        if(impl_->display) impl_->display->reset(reason);
    }

    const DetectorConfig &Detector::config() const noexcept
    {
        return impl_->config;
    }

} // namespace mark
