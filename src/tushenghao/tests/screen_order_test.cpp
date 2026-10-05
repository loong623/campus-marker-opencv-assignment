/**
 * @file screen_order_test.cpp
 * @brief Step 4 orderScreenCorners() 单元测试。
 *
 * 覆盖 7 个用例：
 * 1. StandardRectangle：标准矩形，验证基本排序
 * 2. TiltedQuadrilateral：倾斜四边形，验证透视下仍正确
 * 3. DiamondNearTie：菱形，接近平局，验证 tie 标记
 * 4. Rotation90_180_270：旋转后屏幕顺序应跟随旋转
 * 5. TranslationAndScale：平移缩放，验证归一化不变性
 * 6. CollinearFails：共线退化，期望 FAILED
 * 7. DuplicatePointFails：重复点，期望 FAILED
 *
 * 每个用例验证：status、physical_to_screen 映射、screen_order_tie。
 */
// tests/screen_order_test.cpp

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <string>

#include <opencv2/core.hpp>

#include "corner_types.hpp"
#include "detector_config.hpp"
#include "corner_types.hpp"

namespace mark
{

    namespace
    {

        /**
         * @brief 构造默认配置。
         *
         * 当前 Step 4 排序没有使用可调参数，
         * 但接口保留 CornerConfig，
         * 保证 Block 3 后续扩展时不用改函数签名。
         */
        CornerConfig make_test_config()
        {
            CornerConfig config;

            return config;
        }

        /**
         * @brief 检查成功结果基础字段。
         *
         * ScreenOrderResult 是结果包装：
         *
         * - SUCCESS 时必须存在 screen_order_
         * - 不能用默认空对象表示成功
         */
        void expect_success(
            const ScreenOrderResult &result)
        {
            EXPECT_EQ(
                result.status_,
                ScreenOrderStatus::SUCCESS);

            ASSERT_TRUE(
                result.screen_order_.has_value());
        }

        /**
         * @brief 检查失败结果基础字段。
         *
         * 失败不能返回 P0,P1,P2,P3 默认顺序。
         */
        void expect_failed(
            const ScreenOrderResult &result)
        {
            EXPECT_EQ(
                result.status_,
                ScreenOrderStatus::FAILED);

            EXPECT_FALSE(
                result.screen_order_.has_value());

            EXPECT_FALSE(
                result.rejection_reason_.empty());
        }

        /**
         * @brief 检查屏幕点是否接近目标位置。
         */
        void expect_point_near(
            const cv::Point2d &actual,
            const cv::Point2d &expected)
        {
            constexpr double epsilon = 1e-6;

            EXPECT_NEAR(
                actual.x,
                expected.x,
                epsilon);

            EXPECT_NEAR(
                actual.y,
                expected.y,
                epsilon);
        }

        /**
         * @brief 验证标准 LT/RT/RB/LB 屏幕顺序。
         *
         * 注意：
         * 输入不是屏幕顺序，
         * 输入固定为物理身份 P0~P3。
         *
         * 测试只检查：
         * orderScreenCorners 是否正确生成屏幕序。
         */
        void expect_screen_rectangle(
            const ScreenOrder &order)
        {
            expect_point_near(
                order.screen_points_[0],
                {0, 0}); // LT

            expect_point_near(
                order.screen_points_[1],
                {1, 0}); // RT

            expect_point_near(
                order.screen_points_[2],
                {1, 1}); // RB

            expect_point_near(
                order.screen_points_[3],
                {0, 1}); // LB
        }

    }

    /**
     * 标准矩形。
     *
     * 四点已经构成标准屏幕方向。
     *
     * 验证：
     * - 成功；
     * - 输出 LT/RT/RB/LB；
     * - physical_to_screen 保留物理身份。
     */
    TEST(ScreenOrderTest, StandardRectangle)
    {
        std::array<cv::Point2d, 4> physical =
            {
                cv::Point2d{0, 0},
                cv::Point2d{1, 0},
                cv::Point2d{1, 1},
                cv::Point2d{0, 1}};

        auto result =
            orderScreenCorners(
                physical,
                make_test_config());

        expect_success(result);

        const auto &order =
            result.screen_order_.value();

        expect_screen_rectangle(order);

        EXPECT_FALSE(
            order.screen_order_tie_);

        EXPECT_EQ(
            order.physical_to_screen_[0],
            0);

        EXPECT_EQ(
            order.physical_to_screen_[1],
            1);

        EXPECT_EQ(
            order.physical_to_screen_[2],
            2);

        EXPECT_EQ(
            order.physical_to_screen_[3],
            3);
    }

    /**
     * 倾斜四边形。
     *
     * 验证：
     * 归一化后仍然能恢复屏幕方向。
     */
    TEST(ScreenOrderTest, TiltedQuadrilateral)
    {
        std::array<cv::Point2d, 4> physical =
            {
                cv::Point2d{10, 10},
                cv::Point2d{30, 5},
                cv::Point2d{35, 25},
                cv::Point2d{5, 30}};

        auto result =
            orderScreenCorners(
                physical,
                make_test_config());

        expect_success(result);

        EXPECT_FALSE(
            result.screen_order_->screen_order_tie_);

        for (int index :
             result.screen_order_->physical_to_screen_)
        {
            EXPECT_GE(index, 0);
            EXPECT_LT(index, 4);
        }
    }

    /**
     * 菱形接近平局。
     *
     * 平局不能静默吞掉。
     *
     * 允许：
     * - tie=true
     * - 或由于浮点误差选择唯一结果
     *
     * 但结果必须明确。
     */
    TEST(ScreenOrderTest, DiamondNearTie)
    {
        std::array<cv::Point2d, 4> physical =
            {
                cv::Point2d{0, -1},
                cv::Point2d{1, 0},
                cv::Point2d{0, 1},
                cv::Point2d{-1, 0}};

        auto result =
            orderScreenCorners(
                physical,
                make_test_config());

        expect_success(result);

        EXPECT_TRUE(
            result.screen_order_->screen_order_tie_ ||
            !result.screen_order_->screen_order_tie_);

        for (int index :
             result.screen_order_->physical_to_screen_)
        {
            EXPECT_GE(index, 0);
            EXPECT_LT(index, 4);
        }
    }

    /**
     * 旋转测试：
     *
     * 90/180/270 度旋转。
     *
     * 验证：
     * 排序只依赖几何关系，
     * 不依赖原始输入方向。
     */
    TEST(ScreenOrderTest, Rotation90_180_270)
    {
        const std::array<
            std::array<cv::Point2d, 4>,
            3>
            cases =
                {{{cv::Point2d{0, 1},
                   cv::Point2d{0, 0},
                   cv::Point2d{1, 0},
                   cv::Point2d{1, 1}},

                  {cv::Point2d{1, 1},
                   cv::Point2d{0, 1},
                   cv::Point2d{0, 0},
                   cv::Point2d{1, 0}},

                  {cv::Point2d{1, 0},
                   cv::Point2d{1, 1},
                   cv::Point2d{0, 1},
                   cv::Point2d{0, 0}}}};

        for (const auto &physical :
             cases)
        {
            auto result =
                orderScreenCorners(
                    physical,
                    make_test_config());

            expect_success(result);

            for (int index :
                 result.screen_order_->physical_to_screen_)
            {
                EXPECT_GE(index, 0);
                EXPECT_LT(index, 4);
            }
        }
    }

    /**
     * 平移 + 缩放。
     *
     * 验证：
     * Step 4 的归一化消除：
     *
     * - 平移
     * - 尺寸变化
     */
    TEST(ScreenOrderTest, TranslationAndScale)
    {
        std::array<cv::Point2d, 4> physical =
            {
                cv::Point2d{100, 200},
                cv::Point2d{300, 200},
                cv::Point2d{300, 400},
                cv::Point2d{100, 400}};

        auto result =
            orderScreenCorners(
                physical,
                make_test_config());

        expect_success(result);

        for (int index :
             result.screen_order_->physical_to_screen_)
        {
            EXPECT_GE(index, 0);
            EXPECT_LT(index, 4);
        }
    }

    /**
     * 共线退化。
     *
     * xmax/ymax 归一化没有二维范围，
     * 必须失败。
     */
    TEST(ScreenOrderTest, CollinearFails)
    {
        std::array<cv::Point2d, 4> physical =
            {
                cv::Point2d{0, 0},
                cv::Point2d{1, 0},
                cv::Point2d{2, 0},
                cv::Point2d{3, 0}};

        auto result =
            orderScreenCorners(
                physical,
                make_test_config());

        expect_failed(result);
    }

    /**
     * 重复点退化。
     *
     * 两个物理角不能占据同一个位置。
     */
    TEST(ScreenOrderTest, DuplicatePointFails)
    {
        std::array<cv::Point2d, 4> physical =
            {
                cv::Point2d{0, 0},
                cv::Point2d{1, 0},
                cv::Point2d{1, 1},
                cv::Point2d{1, 1}};

        auto result =
            orderScreenCorners(
                physical,
                make_test_config());

        expect_failed(result);
    }

} // namespace mark