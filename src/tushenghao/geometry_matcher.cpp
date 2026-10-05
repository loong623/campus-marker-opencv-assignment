#include "geometry_matcher.hpp"

#include <algorithm>
#include <array>
#include <set>

#include <opencv2/calib3d.hpp>

namespace mark
{

    namespace
    {

        // Step 6 搜索资源限制。
        // 防止异常场景下组合数量无限增长。
        // constexpr std::size_t MAX_HYPOTHESIS_COUNT = 1000;

        /*
         * 判断模型 polygon 是否属于 L 类型。
         *
         * 注意：
         * 这里使用的是 MarkerGeometry 中 polygon.id 的命名约定。
         *
         * 例如：
         *     L0
         *     L1
         *     L2
         *
         * 不是 detector 已知 MARK ID。
         *
         * Step 6 的目标仍然是：
         *
         * observation
         *      |
         *      v
         * 几何推理
         *      |
         *      v
         * 得到解释
         */
        bool isModelLComponent(
            const GeometryPolygon &polygon)
        {
            return !polygon.id.empty() &&
                   polygon.id[0] == 'L';
        }

        /*
         * 判断观测是否可能属于 L。
         *
         * supported_classes_ 是 Step 5 提供的候选类别，
         * 这里只作为搜索剪枝，不作为最终结论。
         */
        bool isObservationLComponent(
            const ShapeObservation &observation)
        {
            for (const auto &cls :
                 observation.supported_classes_)
            {
                if (cls == "L")
                {
                    return true;
                }
            }

            return false;
        }

    } // namespace

    /*
     * Step 6.2：
     *
     * 三个 L 搜索 + 六种模型对应。
     */
    std::vector<std::vector<ComponentAssignment>>
    generateSixComponentCombinations(
        const std::vector<ShapeObservation> &observations,
        const MarkerGeometry &model_geometry,
        const GeometryConfig& config,
        bool &resource_truncated)
    {
        std::vector<std::vector<ComponentAssignment>>
            combinations;

        resource_truncated = false;

        // debug
        // std::cerr << "[DEBUG] model polygons: " << model_geometry.polygons.size() << std::endl;

        /*
         * 找模型中的三个 L。
         *
         * 注意：
         * L 来自模型配置命名约定，
         * 不是 detector 输入。
         */
        std::vector<std::string> model_L_ids;

        for (const auto &polygon :
             model_geometry.polygons)
        {
            if (isModelLComponent(polygon))
            {
                model_L_ids.push_back(
                    polygon.id);
            }
        }

        // MARK 模型必须存在三个 L 组件。
        if (model_L_ids.size() != 3)
        {
            return combinations;
        }

        /*
         * 找所有可能是 L 的观测。
         */
        std::vector<std::size_t>
            observation_L_indices;

        for (std::size_t i = 0;
             i < observations.size();
             ++i)
        {
            if (isObservationLComponent(
                    observations[i]))
            {
                observation_L_indices.push_back(i);
            }
        }

        /*
         * C(n,3)
         *
         * 三个观测 L 组成一组候选。
         */
        for (std::size_t i = 0;
             i < observation_L_indices.size();
             ++i)
        {
            for (std::size_t j = i + 1;
                 j < observation_L_indices.size();
                 ++j)
            {
                for (std::size_t k = j + 1;
                     k < observation_L_indices.size();
                     ++k)
                {

                    std::array<std::size_t, 3>
                        observation_group =
                            {
                                observation_L_indices[i],
                                observation_L_indices[j],
                                observation_L_indices[k]};

                    /*
                     * 三个 L 对应三个模型 L。
                     *
                     * 枚举 3! = 6 种排列。
                     */
                    std::array<int, 3>
                        permutation =
                            {
                                0, 1, 2};

                    do
                    {

                        if (combinations.size() >= config.max_hypothesis_count_)
                        {
                            resource_truncated = true;
                            return combinations;
                        }

                        std::vector<ComponentAssignment>
                            assignment;

                        for (int index = 0;
                             index < 3;
                             ++index)
                        {
                            ComponentAssignment item;

                            /*
                             * 这里产生：
                             *
                             * observation component
                             *          |
                             *          v
                             * model polygon
                             *
                             * 是候选解释，
                             * 不是已知 ID。
                             */
                            item.component_id_ =
                                observation_group[index];

                            item.model_part_id_ =
                                model_L_ids[permutation[index]];

                            assignment.push_back(item);
                        }

                        combinations.push_back(
                            assignment);

                    } while (
                        std::next_permutation(
                            permutation.begin(),
                            permutation.end()));
                }
            }
        }

        return combinations;
    }

    /*
     * Step 6.3：
     *
     * 模型坐标 -> 工作图坐标
     *
     * 拟合二维仿射。
     */
    cv::Mat fitModelToImageAffine(
        const std::vector<ComponentAssignment> &assignment,
        const std::vector<ShapeObservation> &observations,
        const MarkerGeometry &model_geometry)
    {
        std::vector<cv::Point2f>
            model_points;

        std::vector<cv::Point2f>
            image_points;

        for (const auto &item :
             assignment)
        {

            auto model_it =
                std::find_if(
                    model_geometry.polygons.begin(),
                    model_geometry.polygons.end(),
                    [&](const GeometryPolygon &polygon)
                    {
                        return polygon.id ==
                               item.model_part_id_;
                    });

            if (model_it ==
                model_geometry.polygons.end())
            {
                continue;
            }

            if (item.component_id_ >=
                observations.size())
            {
                continue;
            }

            const auto &observation =
                observations[item.component_id_];

            /*
             * 没有 anchor 的观测不能参与 affine。
             *
             * 不允许：
             * - 质心代替
             * - bbox 中心代替
             */
            if (!observation.anchor_vertex_index_
                     .has_value())
            {
                continue;
            }

            const std::size_t vertex_index =
                observation.anchor_vertex_index_.value();

            if (vertex_index >=
                observation.simplified_polygon_.size())
            {
                continue;
            }

            model_points.push_back(
                model_it->anchor);

            image_points.push_back(
                observation
                    .simplified_polygon_
                        [vertex_index]);
        }

        // debug
        // std::cerr << "[DEBUG] affine points: " << model_points.size() << std::endl;

        /*
         * 二维仿射至少需要三个点。
         */
        if (model_points.size() < 3)
        {
            return cv::Mat();
        }

        // debug
        // cv::Mat result = cv::estimateAffine2D(model_points, image_points);
        // std::cerr << "[DEBUG] affine empty: " << result.empty() << std::endl;
        // return result;

        return cv::estimateAffine2D(
            model_points,
            image_points);
    }

    /*
     * Step 6.4：
     *
     * 生成 GeometryHypothesis。
     */
    GeometryHypothesis buildGeometryHypothesis(
        const std::vector<ComponentAssignment> &assignment,
        const cv::Mat &affine_transform,
        const std::vector<ShapeObservation> &observations,
        const MarkerGeometry &model_geometry)
    {
        GeometryHypothesis hypothesis;

        hypothesis.assignments_ =
            assignment;

        hypothesis.affine_transform_ =
            affine_transform.clone();

        /*
         * 当前版本尚未冻结 residual 计算规则。
         *
         * 保留字段，不伪造评分。
         */
        hypothesis.validation_residual_ =
            0.0;

        hypothesis.completeness_ =
            GeometryCompleteness::PENDING_VALIDATION;

        hypothesis.evidence_
            .push_back(
                "generated from three L component correspondence");

        return hypothesis;
    }

    /*
     * Step 6 总入口。
     */
    GeometryBatch generateGeometryHypotheses(
        const std::vector<ShapeObservation> &observations,
        const MarkerGeometry &model_geometry,
        const GeometryConfig &config)
    {
        GeometryBatch batch;

        bool resource_truncated = false;

        auto combinations =
            generateSixComponentCombinations(
                observations,
                model_geometry,
                config,
                resource_truncated);

        if (resource_truncated)
        {
            batch.resource_truncated_ = true;

            batch.diagnostics_
                .push_back(
                    "geometry hypothesis search truncated by resource limit");
        }

        for (const auto &combination :
             combinations)
        {
            cv::Mat affine =
                fitModelToImageAffine(
                    combination,
                    observations,
                    model_geometry);

            if (affine.empty())
            {
                continue;
            }

            GeometryHypothesis hypothesis =
                buildGeometryHypothesis(
                    combination,
                    affine,
                    observations,
                    model_geometry);

            batch.hypotheses_
                .push_back(
                    hypothesis);
        }

        if (batch.hypotheses_.empty())
        {
            batch.diagnostics_
                .push_back(
                    "no valid geometry hypothesis generated");
        }

        return batch;
    }

} // namespace mark