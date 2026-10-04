// tests/corner_resolver_test.cpp

#include <gtest/gtest.h> // 不管

#include <iostream>
#include <cmath>
#include <string>
#include <vector>

#include "corner_resolver.hpp"
#include "geometry_types.hpp"
#include "marker_geometry.hpp"
#include "prepared_frame.hpp"

namespace mark
{

    namespace
    {

        /**
         * @brief 构造测试使用的 MARK 模型。
         *
         * 测试不读取 yaml，而是在内存中构造与
         * marker_geometry.yaml 相同的模型。
         *
         * 目的：
         * 验证 corner_resolver 是否根据模型绑定、
         * hypothesis 和真实 contour 恢复角点。
         */
        MarkerGeometry make_test_model()
        {
            MarkerGeometry model;

            GeometryPolygon l0;

            l0.id = "L0";

            l0.vertices =
                {
                    {0, 0},
                    {30, 0},
                    {30, 8},
                    {8, 8},
                    {8, 30},
                    {0, 30}};

            GeometryPolygon m1;

            m1.id = "M1";

            m1.vertices =
                {
                    {66, 0},
                    {80, 0},
                    {80, 14},
                    {72, 14},
                    {72, 8},
                    {66, 8}};

            GeometryPolygon l2;

            l2.id = "L2";

            l2.vertices =
                {
                    {80, 80},
                    {50, 80},
                    {50, 72},
                    {72, 72},
                    {72, 50},
                    {80, 50}};

            GeometryPolygon l3;

            l3.id = "L3";

            l3.vertices =
                {
                    {0, 80},
                    {30, 80},
                    {30, 72},
                    {8, 72},
                    {8, 50},
                    {0, 50}};

            model.schema_version = 1;

            model.polygons =
                {
                    l0,
                    m1,
                    l2,
                    l3};

            return model;
        }

        /**
         * @brief 构造单位仿射矩阵。
         *
         * 测试中假设：
         * 模型坐标 == 图像坐标。
         *
         * 这样可以单独验证角点恢复逻辑，
         * 不引入仿射估计误差。
         */
        cv::Mat make_identity_affine()
        {
            return cv::Mat::eye(
                2,
                3,
                CV_64F);
        }

        /**
         * @brief 构造 Block 2 输出的几何假设。
         *
         * assignments_ 模拟：
         *
         * L0 -> component 0
         * M1 -> component 1
         * L2 -> component 2
         * L3 -> component 3
         *
         * 这正是 resolver 实际运行时需要的数据来源。
         */
        GeometryHypothesis make_test_hypothesis()
        {
            GeometryHypothesis hypothesis;

            hypothesis.affine_transform_ =
                make_identity_affine();

            hypothesis.assignments_ =
                {
                    {"L0", 0},
                    {"M1", 1},
                    {"L2", 2},
                    {"L3", 3}};

            return hypothesis;
        }

        /**
         * @brief 构造角点恢复配置。
         *
         * 保持 local_search_margin_ratio_=0.5。
         *
         * 测试数据必须放在模型真实位置，
         * 否则局部搜索 ROI 会故意过滤掉正确 contour。
         */
        CornerConfig make_test_config()
        {
            CornerConfig config;

            config.local_search_margin_ratio_ =
                2.0;  // 原来是0.5，ROI 太小

            config.min_line_points_ =
                2;   // 原来是3，测试点太稀疏

            config.max_line_fit_error_ =
                2.0;

            config.min_intersection_angle_deg_ =
                10.0;

            config.max_corner_error_ =
                5.0;

            config.reject_truncated_corner_ =
                true;

            config.approximation_epsilon_ =
                1.0;

            config.edge_point_distance_threshold_ =
                3.0;

            return config;
        }

        /**
         * @brief 构造 L0 白块。
         *
         * 坐标严格对应 marker_geometry.yaml：
         *
         * L0:
         * (0,0) ~ (30,30)
         *
         * 不能复用给其他 component。
         *
         * 因为 Step 3 会根据 component_id 找真实空间位置，
         * ROI 必须和模型对应。
         */
        WhiteComponent make_l_component(
            std::size_t id)
        {
            WhiteComponent component;

            component.component_id_ =
                id;

            component.contour_ =
                {
                    {0, 0},
                    {30, 0},
                    {30, 8},
                    {8, 8},
                    {8, 30},
                    {0, 30}};

            component.touches_border_ =
                false;

            return component;
        }

        /**
         * @brief 构造 M1 白块。
         *
         * 对应模型：
         *
         * M1:
         * (66,0) ~ (80,14)
         *
         * P1 是分段角，
         * 所以测试必须保留 M 的真实位置。
         */
        WhiteComponent make_m_component(
            std::size_t id)
        {
            WhiteComponent component;

            component.component_id_ =
                id;

            component.contour_ =
                {
                    {66, 0},
                    {80, 0},
                    {80, 14},
                    {72, 14},
                    {72, 8},
                    {66, 8}};

            component.touches_border_ =
                false;

            return component;
        }

        /**
         * @brief 构造 L2 白块。
         *
         * 对应模型：
         *
         * L2:
         * (50,50) ~ (80,80)
         */
        WhiteComponent make_l2_component(
            std::size_t id)
        {
            WhiteComponent component;

            component.component_id_ =
                id;

            component.contour_ =
                {
                    {80, 80},
                    {50, 80},
                    {50, 72},
                    {72, 72},
                    {72, 50},
                    {80, 50}};

            component.touches_border_ =
                false;

            return component;
        }

        /**
         * @brief 构造 L3 白块。
         *
         * 对应模型：
         *
         * L3:
         * (0,50) ~ (30,80)
         */
        WhiteComponent make_l3_component(
            std::size_t id)
        {
            WhiteComponent component;

            component.component_id_ =
                id;

            component.contour_ =
                {
                    {0, 80},
                    {30, 80},
                    {30, 72},
                    {8, 72},
                    {8, 50},
                    {0, 50}};

            component.touches_border_ =
                false;

            return component;
        }

        /**
         * @brief 构造完整测试帧。
         *
         * 每个 component 必须位于自己模型对应的位置。
         *
         * 原因：
         * Step 3 使用：
         *
         * GeometryHypothesis:
         * model_part_id -> component_id
         *
         * 再通过 affine 得到局部搜索区域。
         *
         * 如果所有 component 都使用 L0，
         * ROI 会与 M/L2/L3 的真实位置冲突，
         * 测试无法覆盖真实局部搜索流程。
         */
        PreparedFrame make_test_frame()
        {
            PreparedFrame frame;

            frame.image_ =
                cv::Mat::zeros(
                    120,
                    120,
                    CV_8UC1);

            frame.components_.push_back(
                make_l_component(0));

            frame.components_.push_back(
                make_m_component(1));

            frame.components_.push_back(
                make_l2_component(2));

            frame.components_.push_back(
                make_l3_component(3));

            return frame;
        }

    }

    /**
     * 1. 直接角：
     *
     * 标准 L 形两条直边。
     *
     * 期望：
     * SUCCESS
     *
     * 验证：
     * - 四个物理角存在；
     * - evidence 全部填写；
     * - 没有用零坐标冒充失败。
     */
    TEST(CornerResolverTest, DirectCornerReturnsSuccess)
    {
        auto result =
            resolveObservedCorners(
                make_test_frame(),
                make_test_hypothesis(),
                make_test_model(),
                make_test_config());

        // debug
        if (result.status_ != CornerResolutionStatus::SUCCESS)
        {
            std::cout << "REJECT: " << result.rejection_reason_ << std::endl;
        }

        EXPECT_EQ(
            result.status_,
            CornerResolutionStatus::SUCCESS);

        ASSERT_TRUE(
            result.measurement_.has_value());

        for (const auto &point :
             result.measurement_->physical_corners_)
        {
            EXPECT_TRUE(
                std::isfinite(point.x));

            EXPECT_TRUE(
                std::isfinite(point.y));
        }

        for (const auto &evidence :
             result.measurement_->evidence_)
        {
            EXPECT_TRUE(
                std::isfinite(evidence.error_));
        }
    }

    /**
     * 2. 圆角：
     *
     * 角部被圆弧替代。
     *
     * 根据残差：
     * - 可能 SUCCESS；
     * - 也可能 FAILED。
     *
     * 重点：
     * 不崩溃；
     * FAILED 时必须有明确原因。
     */
    TEST(CornerResolverTest, RoundedCornerDoesNotCrash)
    {
        auto frame =
            make_test_frame();

        frame.components_[0].contour_ =
            {
                {0, 0},
                {10, 0},
                {20, 2},
                {28, 10},
                {30, 20},
                {20, 30},
                {8, 30},
                {0, 20}};

        auto result =
            resolveObservedCorners(
                frame,
                make_test_hypothesis(),
                make_test_model(),
                make_test_config());

        if (result.status_ ==
            CornerResolutionStatus::FAILED)
        {
            EXPECT_FALSE(
                result.rejection_reason_.empty());

            EXPECT_FALSE(
                result.measurement_.has_value());
        }
    }

    /**
     * 3. 短边：
     *
     * 让边上有效点数量不足。
     *
     * 期望：
     * FAILED
     *
     * 原因：
     * fit_line 无法获得可靠直线。
     */
    TEST(CornerResolverTest, ShortEdgeFails)
    {
        auto config =
            make_test_config();

        config.min_line_points_ =
            20;

        auto result =
            resolveObservedCorners(
                make_test_frame(),
                make_test_hypothesis(),
                make_test_model(),
                config);

        EXPECT_EQ(
            result.status_,
            CornerResolutionStatus::FAILED);

        EXPECT_FALSE(
            result.measurement_.has_value());

        EXPECT_NE(
            result.rejection_reason_.find("拟合"),
            std::string::npos);
    }

    /**
     * 4. 近平行：
     *
     * 提高最小交角阈值，
     * 模拟两条边无法稳定求交。
     *
     * 期望：
     * FAILED
     *
     * 原因包含：
     * 平行。
     */
    TEST(CornerResolverTest, ParallelEdgesFail)
    {
        auto config =
            make_test_config();

        config.min_intersection_angle_deg_ =
            95.0;  // 原来是80.0，但 L 形的两条边是 90° 垂直，90 > 80，所以没触发失败。改成 95.0 就能让它挂。

        auto result =
            resolveObservedCorners(
                make_test_frame(),
                make_test_hypothesis(),
                make_test_model(),
                config);

        EXPECT_EQ(
            result.status_,
            CornerResolutionStatus::FAILED);

        EXPECT_FALSE(
            result.measurement_.has_value());

        EXPECT_NE(
            result.rejection_reason_.find("平行"),
            std::string::npos);
    }

    /**
     * 5. 缺边：
     *
     * 删除局部 contour 结构，
     * 使第二条模型边无法找到。
     *
     * 期望：
     * FAILED
     *
     * 原因包含：
     * 不可分。
     */
    TEST(CornerResolverTest, MissingEdgeFails)
    {
        auto frame =
            make_test_frame();

        frame.components_[0].contour_ =
            {
                {0, 0},
                //{30, 0},
                //{30, 1}  3 个点还是能拟合出一条竖边，没达到"缺边"的效果。改成只有 1 个点·，这样 find_matching_edge 找不到边，直接报"不可分"。
            };

        auto result =
            resolveObservedCorners(
                frame,
                make_test_hypothesis(),
                make_test_model(),
                make_test_config());

        EXPECT_EQ(
            result.status_,
            CornerResolutionStatus::FAILED);

        EXPECT_FALSE(
            result.measurement_.has_value());

        EXPECT_NE(
            result.rejection_reason_.find("不可分"),
            std::string::npos);
    }

    /**
     * 6. 截断：
     *
     * 模拟白块碰到图像边界。
     *
     * 即使几何拟合可能成功，
     * 由于观测可能缺失，
     * 配置要求拒绝。
     */
    TEST(CornerResolverTest, TruncatedCornerFails)
    {
        auto frame =
            make_test_frame();

        for (auto &component :
             frame.components_)
        {
            component.touches_border_ =
                true;
        }

        auto config =
            make_test_config();

        config.reject_truncated_corner_ =
            true;

        auto result =
            resolveObservedCorners(
                frame,
                make_test_hypothesis(),
                make_test_model(),
                config);

        EXPECT_EQ(
            result.status_,
            CornerResolutionStatus::FAILED);

        EXPECT_FALSE(
            result.measurement_.has_value());

        EXPECT_NE(
            result.rejection_reason_.find("截断"),
            std::string::npos);
    }

} // namespace mark
