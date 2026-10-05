#include "detector.hpp"
#include "app_config.hpp"
#include "config.hpp"
#include "marker_geometry.hpp"
#include "preprocess.hpp"
#include "geometry_observation.hpp"
#include "geometry_matcher.hpp"
#include "geometry_validation.hpp"
#include "corner_resolver.hpp"
#include "semantic_resolver.hpp"
#include "detection_validator.hpp"
namespace mark
{

    struct Detector::Impl
    {
        DetectorConfig config;

        // Block 2 MARK 几何模型
        MarkerGeometry marker_geometry; // 注意名字是 marker_geometry，后面构造用

        // TODO:
        // 后续板块补充检测状态，例如时序缓存、跟踪状态等。
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
        prepared.components_ = components; // 把 extractWhiteComponents() 刚算出来的白块列表，存进 prepared.components_(顺手配套修改)

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

        // std::cerr << "[DEBUG] hypotheses: " << batch.hypotheses_.size() << std::endl;
        // Block 3 未实现，保持 NOT_READY。
        // result.status = Status::NOT_READY;
        // TODO(调试)：看 Block 2 产出几个假设
        // std::cerr << "[DEBUG] components: " << components.size()
        //           << " observations: " << observations.size()
        //           << " hypotheses: " << batch.hypotheses_.size() << std::endl;

        // return result;

        /*
         * Block 3 集成：
         *
         * Block 2 输出的是“可能解释”：
         *
         * GeometryHypothesis
         *
         * Block 3 不重新检测，
         * 只把已有几何解释转换成最终检测结果。
         *
         * 流程：
         *
         * hypothesis
         *      |
         *      v
         * CornerMeasurement
         *      |
         *      v
         * ScreenOrder
         *      |
         *      v
         * DetectionValidation
         *
         * Step 5 semantic 单独批量处理，
         * 因为它解决的是多个竞争解释之间的关系。
         */

        std::vector<CornerMeasurement> measurements;

        /*
         * Step 1-3:
         * 对每个 Block 2 合格假设恢复四个物理角。
         */
        for (const auto &hypothesis :
             batch.hypotheses_)
        {
            CornerResolution corner_result =
                resolveObservedCorners(
                    prepared,
                    hypothesis,
                    impl_->marker_geometry,
                    impl_->config.corner_);

            if (corner_result.status_ ==
                CornerResolutionStatus::FAILED)
            {
                // std::cerr << "[DEBUG] Block3 reject: resolveObservedCorners FAILED" << std::endl;
                // std::cerr << "[DEBUG] Block3 reject: resolveObservedCorners FAILED: " << corner_result.rejection_reason_ << std::endl;
                continue;
            }

            measurements.push_back(
                *corner_result.measurement_);
        }

        /*
         * Step 5:
         *
         * 多个 CornerMeasurement 是竞争解释，
         * 交给 semantic resolver 判断：
         *
         * - 是否同一几何；
         * - 方向是否唯一；
         * - 保留哪个解释。
         */
        SemanticResolution semantic_result =
            resolveSemantics(
                measurements,
                batch.resource_truncated_,
                impl_->config.corner_);

        if (!semantic_result.geometry_consistent_ ||
            semantic_result.retained_measurements_.empty())
        {
            // std::cerr << "[DEBUG] Block3 reject: resolveSemantics FAILED" << std::endl;
            result.status =
                Status::NOT_DETECTED;

            // result.tracks = result.detections;
            // Block 4 未实现，占位：
            // 稳定结果暂时直接等于当前检测（无时序平滑）。
            // TODO(Block 4)：替换为真正的时序稳定逻辑。
            result.tracks.clear();
            for (size_t i = 0; i < result.detections.size(); ++i)
            {
                TrackResult track;
                track.detection_index = i;
                track.result = result.detections[i];
                result.tracks.push_back(track);
            }

            return result;
        }

        /*
         * Step 4 + Step 6:
         *
         * 对保留的测量建立屏幕顺序，
         * 并进行最终几何质检。
         */
        for (const auto &measurement :
             semantic_result.retained_measurements_)
        {
            ScreenOrderResult order_result =
                orderScreenCorners(
                    measurement.physical_corners_,
                    impl_->config.corner_);

            if (order_result.status_ ==
                ScreenOrderStatus::FAILED)
            {
                // std::cerr << "[DEBUG] Block3 reject: reorder corners FAILED" << std::endl;
                continue;
            }

            DetectionValidation validation =
                validateDetectionGeometry(
                    measurement,
                    *order_result.screen_order_,
                    prepared.image_.size(),
                    impl_->config.corner_);

            if (!validation.valid_)
            {
                // std::cerr << "[DEBUG] Block3 reject: Detection geometry validation FAILED" << std::endl;
                continue;
            }

            /*
             * 通过 Block 3：
             *
             * 生成 Detector 对外输出。
             *
             * 不创建新的角点来源，
             * 直接使用已经验证过的测量结果。
             */
            Detection detection;

            detection.category =
                MarkCategory::Unknown;

            for (int i = 0;
                 i < 4;
                 ++i)
            {
                const int screen_index =
                    order_result
                        .screen_order_
                        ->physical_to_screen_[i];

                const cv::Point2d point =
                    measurement.physical_corners_[i];

                detection.corners[screen_index] =
                    cv::Point2f(
                        static_cast<float>(point.x),
                        static_cast<float>(point.y));
            }

            detection.attributes.orientation =
                order_result
                    .screen_order_
                    ->physical_to_screen_;

            detection.quality_flags = 0;

            result.detections.push_back(
                detection);
        }

        if (!result.detections.empty())
        {
            result.status =
                Status::DETECTED;
        }
        else
        {
            result.status =
                Status::NOT_DETECTED;
        }

        /*
         * Block 4 未实现。
         *
         * 当前只占位：
         * 稳定结果暂时等于当前检测。
         */
        result.tracks.clear();

        for (size_t i = 0;
             i < result.detections.size();
             ++i)
        {
            TrackResult track;

            track.detection_index = i;

            track.result =
                result.detections[i];

            result.tracks.push_back(track);
        }

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