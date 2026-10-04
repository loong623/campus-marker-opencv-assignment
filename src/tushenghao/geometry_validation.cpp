/* 定位：
Step 7 的质检员。
Step 6 一下吐出好几个假设（"这三个白块可能是这样对应，也可能是那样"），这个文件逐个验：投影对得上吗？面积合理吗？缺块吗？
错的扔掉，残的标记，好的送给 Block 3。
*/
#include "geometry_validation.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <opencv2/imgproc.hpp>

namespace mark
{

    namespace
    {

        /*
         * 根据模型 ID 查找模型多边形。
         *
         * 注意：
         * 这里允许出现 L0/L1/L2 等 ID，
         * 因为它们来自 MarkerGeometry 模型文件。
         *
         * 不是 detector 已知答案。
         */
        const GeometryPolygon *findPolygon(
            const MarkerGeometry &geometry,
            const std::string &id)
        {
            for (const auto &polygon :
                 geometry.polygons)
            {
                if (polygon.id == id)
                {
                    return &polygon;
                }
            }

            return nullptr;
        }

        /*
         * 计算多边形顶点平均距离。
         *
         * 用于：
         *
         * 模型投影 polygon
         *          |
         *          v
         *     观测 polygon
         *
         * 的几何一致性检查。
         */
        double polygonResidual(
            const std::vector<cv::Point2f> &projected,
            const std::vector<cv::Point> &observed)
        {
            if (projected.empty() ||
                observed.empty())
            {
                return std::numeric_limits<double>::max();
            }

            const std::size_t count =
                std::min(
                    projected.size(),
                    observed.size());

            double total = 0.0;

            for (std::size_t i = 0;
                 i < count;
                 ++i)
            {
                const double dx =
                    projected[i].x -
                    static_cast<double>(
                        observed[i].x);

                const double dy =
                    projected[i].y -
                    static_cast<double>(
                        observed[i].y);

                total +=
                    std::sqrt(
                        dx * dx +
                        dy * dy);
            }

            return total /
                   static_cast<double>(count);
        }

        /*
         * 阶段 1：
         *
         * 模型投影 vs 观测轮廓
         *
         * 检查 hypothesis 是否几何一致。
         */
        bool checkGeometricConsistency(
            const GeometryHypothesis &hypothesis,
            const MarkerGeometry &geometry,
            const std::vector<WhiteComponent> &components,
            double max_residual)
        {

            for (const auto &assignment :
                 hypothesis.assignments_)
            {

                const auto *polygon =
                    findPolygon(
                        geometry,
                        assignment.model_part_id_);

                if (polygon == nullptr)
                {
                    return false;
                }

                if (assignment.component_id_ >= components.size())
                {
                    return false;
                }

                const auto &component =
                    components[assignment.component_id_];

                std::vector<cv::Point2f>
                    projected;

                cv::transform(
                    polygon->vertices,
                    projected,
                    hypothesis.affine_transform_);

                const double residual =
                    polygonResidual(
                        projected,
                        component.contour_);

                if (residual > max_residual)
                {
                    return false;
                }
            }

            return true;
        }

        /*
         * 计算多边形面积。
         */
        double polygonArea(
            const std::vector<cv::Point2f> &points)
        {
            return std::abs(
                cv::contourArea(points));
        }

        /*
         * 阶段 2：
         *
         * 投影面积 / 观测面积
         *
         * 超范围不直接删除，
         * 由完整性阶段降级。
         */
        bool checkAreaRatio(
            const GeometryHypothesis &hypothesis,
            const MarkerGeometry &geometry,
            const std::vector<WhiteComponent> &components,
            double min_ratio,
            double max_ratio)
        {

            for (const auto &assignment :
                 hypothesis.assignments_)
            {

                const auto *polygon =
                    findPolygon(
                        geometry,
                        assignment.model_part_id_);

                if (polygon == nullptr)
                {
                    return false;
                }

                if (assignment.component_id_ >= components.size())
                {
                    return false;
                }

                const auto &component =
                    components[assignment.component_id_];

                std::vector<cv::Point2f>
                    projected;

                cv::transform(
                    polygon->vertices,
                    projected,
                    hypothesis.affine_transform_);

                const double projected_area =
                    polygonArea(projected);

                if (component.area_ <= 0.0)
                {
                    return false;
                }

                const double ratio =
                    projected_area /
                    component.area_;

                if (ratio < min_ratio ||
                    ratio > max_ratio)
                {
                    return false;
                }
            }

            return true;
        }

        /*
         * 阶段 3：
         *
         * 完整性判断。
         *
         * 这里只判断：
         *
         * CLEARLY_INCOMPLETE
         * PENDING_VALIDATION
         *
         * 不输出概率。
         */
        GeometryCompleteness evaluateCompleteness(
            const GeometryHypothesis &hypothesis,
            const std::vector<WhiteComponent> &components)
        {

            for (const auto &assignment :
                 hypothesis.assignments_)
            {

                if (assignment.component_id_ >= components.size())
                {
                    return GeometryCompleteness::
                        CLEARLY_INCOMPLETE;
                }

                if (components[assignment.component_id_]
                        .touches_border_)
                {
                    return GeometryCompleteness::
                        CLEARLY_INCOMPLETE;
                }
            }

            return GeometryCompleteness::
                PENDING_VALIDATION;
        }

    } // namespace

    GeometryBatch validateGeometryBatch(
        const GeometryBatch &batch,
        const MarkerGeometry &geometry,
        const std::vector<WhiteComponent> &components,
        const GeometryConfig &config)
    {

        GeometryBatch output;

        for (const auto &hypothesis :
             batch.hypotheses_)
        {

            /*
             * 复制。
             *
             * 不修改 Step6 原始结果。
             */
            GeometryHypothesis checked =
                hypothesis;

            if (!checkGeometricConsistency(
                    checked,
                    geometry,
                    components,
                    config.max_validation_residual_))
            {

                output.diagnostics_
                    .push_back(
                        "hypothesis rejected: "
                        "geometric residual too large");

                continue;
            }

            if (!checkAreaRatio(
                    checked,
                    geometry,
                    components,
                    config.min_area_ratio_,
                    config.max_area_ratio_))
            {
                checked.completeness_ =
                    GeometryCompleteness::
                        CLEARLY_INCOMPLETE;

                checked.evidence_
                    .push_back(
                        "area ratio outside validation range");
            }
            else
            {
                checked.completeness_ =
                    evaluateCompleteness(
                        checked,
                        components);
            }

            output.hypotheses_
                .push_back(
                    checked);
        }

        /*
         * Step6 已经可能因为资源限制截断。
         *
         * Step7 保留这个状态，
         * 不重新解释。
         */
        output.resource_truncated_ =
            batch.resource_truncated_;

        return output;
    }

} // namespace mark