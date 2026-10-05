// 把 Block 2 猜的"大概位置"，变成 4 个角的"精确像素坐标"
/* 精确定位器——Block 2 负责"认出来"，它负责"量准了"
1. 输入：Block 2 的 GeometryHypothesis（"我觉得 MARK 在这，形状是这样"）
2. 干啥：对 P0~P3 每个角，去真实图像里找它两条边的像素点，拟合出两条直线，求交点
3. 关键：交点必须来自真实 contour，模型只用来"指方向、框范围"，不许用模型点冒充观测点
4. 输出：4 个角的原图坐标 + 每条边的拟合证据；证据不足就 FAILED，不瞎猜
*/
#include "corner_resolver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace mark
{

    namespace
    {

        // binding、查找、几何辅助
        /**
         * @brief Step 1 固定的物理角绑定关系。
         *
         * 这里保存的是 MARK 自身定义，不是当前帧检测结果。
         *
         * 原因：
         * P0~P3 的物理身份不会因为屏幕旋转改变，
         * 所以不能通过图像位置猜角。
         *
         * 运行时根据这个表去 MarkerGeometry 查询模型顶点和边。
         */
        struct CornerModelBinding
        {
            PhysicalCorner corner_;

            // 对应 marker_geometry.yaml 中的 polygon id。
            std::string polygon_id_;

            // 该物理角在 polygon.vertices 中对应的顶点编号。
            int vertex_index_;

            // 两条相邻模型边编号。
            int edge_a_;
            int edge_b_;
        };

        /**
         * @brief 获取固定物理角绑定。
         *
         * 不从图像、不从预测、不从排序结果生成。
         *
         * 如果这里不存在绑定，说明程序自身配置错误，
         * 不能继续恢复角点。
         */
        std::optional<CornerModelBinding> get_corner_binding(
            PhysicalCorner corner)
        {
            switch (corner)
            {
            case PhysicalCorner::P0:
                return CornerModelBinding{
                    PhysicalCorner::P0,
                    "L0",
                    0,
                    5,
                    0};

            case PhysicalCorner::P1:
                return CornerModelBinding{
                    PhysicalCorner::P1,
                    "M1",
                    1,
                    0,
                    1};

            case PhysicalCorner::P2:
                return CornerModelBinding{
                    PhysicalCorner::P2,
                    "L2",
                    0,
                    5,
                    0};

            case PhysicalCorner::P3:
                return CornerModelBinding{
                    PhysicalCorner::P3,
                    "L3",
                    0,
                    5,
                    0};
            }

            return std::nullopt;
        }

        /**
         * @brief 根据模型编号找到对应几何片段。
         *
         * MarkerGeometry 是模型真值来源。
         *
         * 这里不能根据图像形状重新判断 L/M/S，
         * 因为 Step 3 只负责观察验证。
         */
        const GeometryPolygon *find_polygon(
            const MarkerGeometry &model,
            const std::string &id)
        {
            for (const auto &polygon : model.polygons)
            {
                if (polygon.id == id)
                {
                    return &polygon;
                }
            }

            return nullptr;
        }

        /**
         * @brief 根据 GeometryHypothesis 查找真实白块。
         *
         * hypothesis 只保存：
         * 模型片段 -> component_id。
         *
         * 真正轮廓证据来自 PreparedFrame.components_。
         *
         * 找不到说明：
         * 假设认为存在的结构，没有对应图像证据。
         */
        // const WhiteComponent *find_component(
        //     const PreparedFrame &frame,
        //     const GeometryHypothesis &hypothesis,
        //     const std::string &model_part_id)
        // {
        //     for (const auto &assignment : hypothesis.assignments_)
        //     {
        //         if (assignment.model_part_id_ != model_part_id)
        //         {
        //             continue;
        //         }

        //         for (const auto &component : frame.components_)
        //         {
        //             if (component.component_id_ ==
        //                 assignment.component_id_)
        //             {
        //                 return &component;
        //             }
        //         }
        //     }

        //     return nullptr;
        // }
        const WhiteComponent *find_component(
            const PreparedFrame &frame,
            const GeometryHypothesis &hypothesis,
            const MarkerGeometry &model,
            const std::string &model_part_id)
        {
            // 1. 先在 hypothesis 的 assignment 里找（L0/L2/L3 走这里）
            for (const auto &assignment : hypothesis.assignments_)
            {
                if (assignment.model_part_id_ != model_part_id)
                {
                    continue;
                }
                for (const auto &component : frame.components_)
                {
                    if (component.component_id_ == assignment.component_id_)
                    {
                        return &component;
                    }
                }
            }

            // 2. 2026-10-05 补充：
            // M1 不在 3-L 假设里，但它在图像里真实存在。
            // 用 affine 把模型 M1 的 anchor 投影到图像，
            // 找质心最近的白块（用观测证据，不猜点）。
            const GeometryPolygon *poly = nullptr;
            for (const auto &p : model.polygons)
            {
                if (p.id == model_part_id)
                {
                    poly = &p;
                    break;
                }
            }
            if (poly == nullptr)
            {
                return nullptr;
            }
            // 投影 anchor
            double ax = hypothesis.affine_transform_.at<double>(0, 0) * poly->anchor.x +
                        hypothesis.affine_transform_.at<double>(0, 1) * poly->anchor.y +
                        hypothesis.affine_transform_.at<double>(0, 2);
            double ay = hypothesis.affine_transform_.at<double>(1, 0) * poly->anchor.x +
                        hypothesis.affine_transform_.at<double>(1, 1) * poly->anchor.y +
                        hypothesis.affine_transform_.at<double>(1, 2);
            const WhiteComponent *best = nullptr;
            double best_dist = 1e18;
            for (const auto &c : frame.components_)
            {
                // 用包围盒中心近似质心
                cv::Rect bb = cv::boundingRect(c.contour_);
                double cx = bb.x + bb.width * 0.5;
                double cy = bb.y + bb.height * 0.5;
                double d = (cx - ax) * (cx - ax) + (cy - ay) * (cy - ay);
                if (d < best_dist)
                {
                    best_dist = d;
                    best = &c;
                }
            }
            return best;
        }

        /**
         * @brief 将模型点变换到工作图坐标。
         *
         * affine_transform_ 的方向固定：
         * model -> working image。
         *
         * 这里只用于限制搜索区域和比较方向，
         * 不会生成检测角点。
         */
        cv::Point2d transform_point(
            const cv::Mat &affine,
            const cv::Point2f &point)
        {
            return {
                affine.at<double>(0, 0) * point.x +
                    affine.at<double>(0, 1) * point.y +
                    affine.at<double>(0, 2),

                affine.at<double>(1, 0) * point.x +
                    affine.at<double>(1, 1) * point.y +
                    affine.at<double>(1, 2)};
        }

        /**
         * @brief 获取模型边经过仿射后的图像方向。
         *
         * 边方向用于从真实 contour 中寻找对应观测边。
         *
         * 注意：
         * 方向只是筛选证据，
         * 最终直线仍然由原图 contour 拟合。
         */
        cv::Point2d transform_edge_direction(
            const cv::Mat &affine,
            const cv::Point2f &a,
            const cv::Point2f &b)
        {
            return transform_point(affine, b) -
                   transform_point(affine, a);
        }

        /**
         * @brief 计算两个向量夹角。
         *
         * 使用：
         *
         * cos(theta)=a·b/(|a||b|)
         *
         * 用于判断 contour 边方向是否接近模型边方向。
         */
        double vector_angle(
            const cv::Point2d &a,
            const cv::Point2d &b)
        {
            double length_a =
                std::sqrt(
                    a.x * a.x +
                    a.y * a.y);

            double length_b =
                std::sqrt(
                    b.x * b.x +
                    b.y * b.y);

            if (length_a < 1e-6 ||
                length_b < 1e-6)
            {
                return 180.0;
            }

            double cos_value =
                (a.x * b.x + a.y * b.y) /
                (length_a * length_b);

            cos_value =
                std::clamp(
                    cos_value,
                    -1.0,
                    1.0);

            return std::acos(cos_value) *
                   180.0 /
                   CV_PI;
        }

        // ROI、边提取、拟合、投影
        /**
         * @brief 计算模型角点在图像中的局部搜索区域。
         *
         * Step 3 不能直接在整个 component contour 中盲目找边，
         * 因为一个白块可能包含多个结构边。
         *
         * 这里使用 Block 2 给出的 affine_transform_，
         * 将模型角点投影到工作图，
         * 再根据 margin 扩大搜索范围。
         *
         * 注意：
         * 这个位置只是限制搜索范围，
         * 不是预测角点。
         *
         * 最终角点仍然必须来自真实 contour。
         */
        cv::Rect compute_local_search_roi(
            const cv::Mat &affine,
            const GeometryPolygon &polygon,
            int vertex_index,
            double margin_ratio,
            const cv::Size &image_size)
        {
            cv::Point2d center =
                transform_point(
                    affine,
                    polygon.vertices[vertex_index]);

            // TODO: 50.0 是 ROI 换算系数（ratio 转像素）。
            // 当前 v1 用固定值，调 margin_ratio 已够用。
            // 如果实测发现调 margin_ratio 不够，必须动 50.0，
            // 则升级为第 9 个配置字段，走 5 步流程：
            // hpp 加字段 → yaml 加参数 → config.cpp load/validate/write。
            /*到时候 5 步（跟第 7、8 字段一模一样）：
            1. detector_config.hpp：CornerConfig 加 double roi_scale_{50.0};
            2. detector.yaml：corner: 下加 roi_scale: 50.0
            3. config.cpp loadConfig()：3 参数 readDouble
            4. config.cpp validateConfig()：> 0 + ConfigError
            5. config.cpp writeEffectiveConfig()：fs << + detector.corner_ 前缀
            */
            int margin =
                static_cast<int>(
                    std::max(
                        10.0,
                        margin_ratio * 50.0));

            cv::Rect roi(
                static_cast<int>(center.x) - margin,
                static_cast<int>(center.y) - margin,
                margin * 2,
                margin * 2);

            return roi &
                   cv::Rect(
                       0,
                       0,
                       image_size.width,
                       image_size.height);
        }

        /**
         * @brief 从 ROI 中筛选 contour 点。
         *
         * 原始 contour 是真实观测。
         *
         * ROI 只是减少错误边竞争，
         * 不会产生新的点。
         */
        std::vector<cv::Point> filter_contour_by_roi(
            const std::vector<cv::Point> &contour,
            const cv::Rect &roi)
        {
            std::vector<cv::Point> result;

            for (const auto &point : contour)
            {
                if (roi.contains(point))
                {
                    result.push_back(point);
                }
            }

            return result;
        }

        /**
         * @brief 从 contour 中寻找方向最接近模型边的真实观测点。
         *
         * 流程：
         *
         * 1. approxPolyDP 找到稳定候选边；
         * 2. 根据候选边方向筛选原始 contour 点；
         * 3. 返回这些真实点用于 fitLine。
         *
         * 为什么不用 approx 后的点直接拟合：
         *
         * approx 是简化结果，
         * 会丢失真实边上的像素信息。
         *
         * Step 3 要求恢复观测证据，
         * 所以拟合必须基于原始 contour。
         */
        bool find_matching_edge(
            const std::vector<cv::Point> &contour,
            const cv::Point2d &target_direction,
            double approximation_epsilon,
            double edge_point_distance_threshold,
            std::vector<cv::Point2d> &output_points)
        {
            std::vector<cv::Point> approx;

            cv::approxPolyDP(
                contour,
                approx,
                approximation_epsilon,
                true);

            if (approx.size() < 2)
            {
                return false;
            }

            double best_error =
                std::numeric_limits<double>::max();

            cv::Point2d best_a;
            cv::Point2d best_b;

            // 先找到方向最接近模型边的候选边。
            for (size_t i = 0;
                 i < approx.size();
                 ++i)
            {
                cv::Point2d a =
                    approx[i];

                cv::Point2d b =
                    approx[(i + 1) % approx.size()];

                double error =
                    vector_angle(
                        b - a,
                        target_direction);

                // 直线方向正反都代表同一条边。
                error =
                    std::min(
                        error,
                        180.0 - error);

                if (error < best_error)
                {
                    best_error = error;
                    best_a = a;
                    best_b = b;
                }
            }

            cv::Point2d edge =
                best_b - best_a;

            double edge_length =
                std::sqrt(
                    edge.x * edge.x +
                    edge.y * edge.y);

            if (edge_length < 1e-6)
            {
                return false;
            }

            // 收集原始 contour 中落在该边附近的真实观测点。
            //
            // 点到直线距离公式：
            //
            // d = |(b-a) x (p-a)| / |b-a|
            //
            // 距离小于阈值才认为属于该边。
            for (const auto &raw_point : contour)
            {
                cv::Point2d p =
                    raw_point;

                double distance =
                    std::abs(
                        edge.x * (best_a.y - p.y) -
                        (best_a.x - p.x) * edge.y) /
                    edge_length;

                if (distance <=
                    edge_point_distance_threshold)
                {
                    output_points.push_back(p);
                }
            }

            return !output_points.empty();
        }

        /**
         * @brief 用多个真实边点拟合直线并计算残差。
         *
         * 不能只使用两个端点：
         *
         * - 两点无法反映边上噪声；
         * - 无法判断当前边证据质量。
         *
         * 使用：
         *
         * cv::fitLine(points)
         *
         * 得到：
         *
         * (vx,vy,x0,y0)
         *
         * 再计算所有点到直线平均距离：
         *
         * error = Σdistance(point,line) / N
         *
         * error 作为 CornerEvidence 的几何质量依据。
         */
        bool fit_line(
            const std::vector<cv::Point2d> &points,
            int min_line_points,
            double max_line_fit_error,
            cv::Vec4d &output_line,
            double &output_error)
        {
            if (static_cast<int>(points.size()) <
                min_line_points)
            {
                return false;
            }

            std::vector<cv::Point2f> input;

            for (const auto &point : points)
            {
                input.emplace_back(
                    static_cast<float>(point.x),
                    static_cast<float>(point.y));
            }

            cv::Vec4f line;

            cv::fitLine(
                input,
                line,
                cv::DIST_L2,
                0,
                0.01,
                0.01);

            output_line =
                {
                    line[0],
                    line[1],
                    line[2],
                    line[3]};

            double vx = line[0];
            double vy = line[1];

            double x0 = line[2];
            double y0 = line[3];

            double total_error = 0.0;

            for (const auto &point : points)
            {
                // 点到参数直线距离：
                //
                // |v × (p-p0)|
                //
                double distance =
                    std::abs(
                        vx * (point.y - y0) -
                        vy * (point.x - x0));

                total_error += distance;
            }

            output_error =
                total_error /
                points.size();

            return output_error <=
                   max_line_fit_error;
        }

        /**
         * @brief 从拟合边点中恢复真实边段端点。
         *
         * 不能使用：
         *
         * points.front()
         * points.back()
         *
         * 因为 contour 点顺序不保证对应当前边方向。
         *
         * 方法：
         *
         * 将所有点投影到拟合方向：
         *
         * t = p dot direction
         *
         * 取：
         *
         * min(t), max(t)
         *
         * 对应的两个真实点作为边段端点。
         */
        void project_segment_endpoints(
            const std::vector<cv::Point2d> &points,
            const cv::Vec4d &line,
            cv::Point2d &start,
            cv::Point2d &end)
        {
            cv::Point2d direction{
                line[0],
                line[1]};

            double min_projection =
                std::numeric_limits<double>::max();

            double max_projection =
                -std::numeric_limits<double>::max();

            for (const auto &point : points)
            {
                double projection =
                    point.x * direction.x +
                    point.y * direction.y;

                if (projection < min_projection)
                {
                    min_projection = projection;
                    start = point;
                }

                if (projection > max_projection)
                {
                    max_projection = projection;
                    end = point;
                }
            }
        }

        /**
         * @brief 求两条拟合直线交点。
         *
         * 平行或接近平行时，
         * 交点对噪声高度敏感。
         *
         * 上层会根据角度阈值提前拒绝。
         */
        bool compute_intersection(
            const cv::Vec4d &line_a,
            const cv::Vec4d &line_b,
            cv::Point2d &output)
        {
            double cross =
                line_a[0] * line_b[1] -
                line_a[1] * line_b[0];

            if (std::abs(cross) < 1e-6)
            {
                return false;
            }

            double t =
                ((line_b[2] - line_a[2]) * line_b[1] -
                 (line_b[3] - line_a[3]) * line_b[0]) /
                cross;

            output.x =
                line_a[2] +
                t * line_a[0];

            output.y =
                line_a[3] +
                t * line_a[1];

            return true;
        }

    } // anonymous namespace

    // 主循环、证据填充、失败路径
    CornerResolution resolveObservedCorners(
        const PreparedFrame &frame,
        const GeometryHypothesis &hypothesis,
        const MarkerGeometry &model,
        const CornerConfig &config)
    {
        CornerResolution result;

        result.status_ =
            CornerResolutionStatus::FAILED;

        CornerMeasurement measurement;

        const std::array<PhysicalCorner, 4> corners =
            {
                PhysicalCorner::P0,
                PhysicalCorner::P1,
                PhysicalCorner::P2,
                PhysicalCorner::P3};

        for (size_t index = 0;
             index < corners.size();
             ++index)
        {
            const auto corner =
                corners[index];

            /*
             * 根据固定物理语义取得模型绑定。
             *
             * 这里失败代表程序内部没有完整描述 MARK，
             * 不是视觉检测失败。
             */
            auto binding =
                get_corner_binding(corner);

            if (!binding.has_value())
            {
                result.rejection_reason_ =
                    "物理角绑定不存在";

                return result;
            }

            /*
             * 从 MarkerGeometry 查模型。
             *
             * 不使用当前图像猜测模型片段，
             * 因为 Block 3 只验证 Block 2 的假设。
             */
            const auto *polygon =
                find_polygon(
                    model,
                    binding->polygon_id_);

            if (polygon == nullptr)
            {
                if (corner == PhysicalCorner::P1)
                {
                    result.rejection_reason_ =
                        "M 模型缺失，P1 无法定位";
                }
                else
                {
                    result.rejection_reason_ =
                        "对应模型片段缺失";
                }

                return result;
            }

            /*
             * 根据 GeometryHypothesis 找真实白块。
             *
             * hypothesis 只保存：
             * model_part_id -> component_id。
             *
             * contour 必须来自 PreparedFrame，
             * 不允许用模型投影替代。
             */
            // const auto *component =
            //     find_component(
            //         frame,
            //         hypothesis,
            //         binding->polygon_id_);
            const WhiteComponent *component =
                find_component(
                    frame,
                    hypothesis,
                    model,
                    binding->polygon_id_);

            if (component == nullptr)
            {
                result.rejection_reason_ =
                    "对应白块缺失";

                return result;
            }

            /*
             * 计算该物理角的两条模型边。
             *
             * 边编号规则：
             *
             * e0 = v0 -> v1
             * e1 = v1 -> v2
             *
             * 以此类推。
             */
            int edge_a_next =
                (binding->edge_a_ + 1) %
                static_cast<int>(
                    polygon->vertices.size());

            int edge_b_next =
                (binding->edge_b_ + 1) %
                static_cast<int>(
                    polygon->vertices.size());

            cv::Point2d direction_a =
                transform_edge_direction(
                    hypothesis.affine_transform_,
                    polygon->vertices[binding->edge_a_],
                    polygon->vertices[edge_a_next]);

            cv::Point2d direction_b =
                transform_edge_direction(
                    hypothesis.affine_transform_,
                    polygon->vertices[binding->edge_b_],
                    polygon->vertices[edge_b_next]);

            /*
             * 局部搜索：
             *
             * 用模型角点经过 affine 后的位置确定搜索区域。
             *
             * 这里只限制搜索范围，
             * 不生成预测点。
             */
            cv::Rect roi =
                compute_local_search_roi(
                    hypothesis.affine_transform_,
                    *polygon,
                    binding->vertex_index_,
                    config.local_search_margin_ratio_,
                    frame.image_.size());

            auto local_contour =
                filter_contour_by_roi(
                    component->contour_,
                    roi);

            if (local_contour.empty())
            {
                result.rejection_reason_ =
                    "局部区域没有有效边证据";

                return result;
            }

            std::vector<cv::Point2d> edge_points_a;
            std::vector<cv::Point2d> edge_points_b;

            /*
             * 找两条真实观测边。
             *
             * 找不到说明：
             * 当前假设需要的结构没有被图像支持。
             */
            if (!find_matching_edge(
                    local_contour,
                    direction_a,
                    config.approximation_epsilon_,
                    config.edge_point_distance_threshold_,
                    edge_points_a))
            {
                result.rejection_reason_ =
                    "P" +
                    std::to_string(
                        static_cast<int>(corner)) +
                    " 第一条边不可分";

                return result;
            }

            if (!find_matching_edge(
                    local_contour,
                    direction_b,
                    config.approximation_epsilon_,
                    config.edge_point_distance_threshold_,
                    edge_points_b))
            {
                result.rejection_reason_ =
                    "P" +
                    std::to_string(
                        static_cast<int>(corner)) +
                    " 第二条边不可分";

                return result;
            }

            cv::Vec4d line_a;
            cv::Vec4d line_b;

            double residual_a = 0.0;
            double residual_b = 0.0;

            /*
             * 多点拟合。
             *
             * 点数不足：
             * 边证据太少。
             *
             * 残差过大：
             * 当前像素边不符合直线模型。
             */
            if (!fit_line(
                    edge_points_a,
                    config.min_line_points_,
                    config.max_line_fit_error_,
                    line_a,
                    residual_a))
            {
                result.rejection_reason_ =
                    "直线拟合失败";

                return result;
            }

            if (!fit_line(
                    edge_points_b,
                    config.min_line_points_,
                    config.max_line_fit_error_,
                    line_b,
                    residual_b))
            {
                result.rejection_reason_ =
                    "直线拟合失败";

                return result;
            }

            double angle =
                vector_angle(
                    {line_a[0], line_a[1]},
                    {line_b[0], line_b[1]});

            /*
             * 两条边接近平行时，
             * 交点会因为很小噪声产生巨大偏移。
             */
            if (angle <
                config.min_intersection_angle_deg_)
            {
                result.rejection_reason_ =
                    "两直线近平行，交点不稳定";

                return result;
            }

            cv::Point2d intersection;

            if (!compute_intersection(
                    line_a,
                    line_b,
                    intersection))
            {
                result.rejection_reason_ =
                    "两直线近平行，交点不稳定";

                return result;
            }

            // 工作图坐标转原图坐标
            // preprocess 把原图缩放到工作图，contour_ 是工作图坐标
            // 冻结 3-2.md §4.1/§4.4/§4.8 要求最终四点为原图坐标
            // scale_x_ = work_width / original_width，所以原图 = 工作图 / scale
            intersection.x /= frame.scale_x_;
            intersection.y /= frame.scale_y_;

            // 注意：误差不转原图坐标
            // residual 是工作图上的像素距离，max_line_fit_error_ / max_corner_error_
            // 也是按工作图调的阈值，验证要在同一坐标系下做
            // 只有最终输出的位置（交点、边段）才转原图
            double corner_error =
                std::max(
                    residual_a,
                    residual_b);

            // TODO: max_corner_error_ 目前与 max_line_fit_error_ 冗余
            // 当前实现 corner_error = max(residual_a, residual_b)，
            // 而 max_corner_error_(5.0) 比 max_line_fit_error_(2.0) 松，
            // 实际起作用的是后者。冻结文档未定义 max_corner_error_ 的精确语义，
            // 未来需确认：它是应该检查别的指标（如交点到轮廓距离），
            // 还是就保持冗余作为双保险。
            /*  语义：
            max_line_fit_error_（2.0）：每条线单独检查残差
            max_corner_error_（5.0）：检查两条线里较差的那个残差
            */
            if (corner_error >
                config.max_corner_error_)
            {
                result.rejection_reason_ =
                    "交点误差超过阈值";

                return result;
            }

            /*
             * 恢复真实边段端点。
             *
             * 不能使用 vector.front/back，
             * 因为 contour 点顺序不是拟合方向。
             */
            cv::Point2d segment_a_start;
            cv::Point2d segment_a_end;

            cv::Point2d segment_b_start;
            cv::Point2d segment_b_end;

            project_segment_endpoints(
                edge_points_a,
                line_a,
                segment_a_start,
                segment_a_end);

            project_segment_endpoints(
                edge_points_b,
                line_b,
                segment_b_start,
                segment_b_end);

            // 边段端点转原图坐标（3-2.md 要求 CornerEvidence 含"原图边段"）
            segment_a_start.x /= frame.scale_x_;
            segment_a_start.y /= frame.scale_y_;
            segment_a_end.x /= frame.scale_x_;
            segment_a_end.y /= frame.scale_y_;
            segment_b_start.x /= frame.scale_x_;
            segment_b_start.y /= frame.scale_y_;
            segment_b_end.x /= frame.scale_x_;
            segment_b_end.y /= frame.scale_y_;

            // 直线上的点转原图坐标，方向向量 (vx,vy) 不变
            line_a[2] /= frame.scale_x_;
            line_a[3] /= frame.scale_y_;
            line_b[2] /= frame.scale_x_;
            line_b[3] /= frame.scale_y_;

            CornerEvidence evidence;

            evidence.physical_corner_ =
                corner;

            evidence.edge_segment_a_ =
                {
                    segment_a_start,
                    segment_a_end};

            evidence.edge_segment_b_ =
                {
                    segment_b_start,
                    segment_b_end};

            evidence.line_a_ =
                line_a;

            evidence.line_b_ =
                line_b;

            evidence.intersection_ =
                intersection;

            /*
             * 记录真实拟合误差。
             *
             * 后续 audit 可以知道：
             * 是角点位置错，
             * 还是边本身质量不足。
             */
            evidence.error_ =
                corner_error;

            evidence.truncated_ =
                component->touches_border_;

            /*
             * 图像边界截断意味着：
             * 当前 contour 可能只包含部分结构。
             *
             * 不允许用模型补齐缺失部分。
             */
            if (config.reject_truncated_corner_ &&
                evidence.truncated_)
            {
                result.rejection_reason_ =
                    "角点结构被图像边界截断";

                return result;
            }

            measurement.physical_corners_[index] =
                intersection;

            measurement.evidence_[index] =
                evidence;
        }

        /*
         * 四个物理角全部来自真实观测证据。
         *
         * 成功时 measurement 必须存在，
         * 失败时不会返回半成品。
         */
        result.status_ =
            CornerResolutionStatus::SUCCESS;

        result.measurement_ =
            measurement;

        result.rejection_reason_.clear();

        return result;
    }

} // namespace mark
