// tests/detection_validator_test.cpp
// Block3 Step 6：几何校验实现与测试:
// 8 个用例，验证 Step 6 的 7 条拒绝路径 + 1 条通过路径：合法凸四边形放行；非有限值、映射非法（越界/重复）、退化边、非凸、蝴蝶结、出界全部拒绝并给出中文原因。
/* 当前实现检查顺序：
finite
 ↓
mapping
 ↓
reorder
 ↓
退化
 ↓
凸性
 ↓
自交
*/

#include <limits>

#include <gtest/gtest.h>

#include "detection_validator.hpp"
#include "detector_config.hpp"

namespace mark
{

    namespace
    {

        /**
         * @brief 构造标准合法四边形。
         *
         * physical_corners_ 固定表示：
         *
         * P0
         * P1
         * P2
         * P3
         *
         * 不表示屏幕顺序。
         */
        CornerMeasurement make_valid_measurement()
        {
            CornerMeasurement measurement;

            measurement.physical_corners_ =
                {
                    cv::Point2d{100.0, 100.0},
                    cv::Point2d{200.0, 100.0},
                    cv::Point2d{200.0, 200.0},
                    cv::Point2d{100.0, 200.0}};

            return measurement;
        }

        /**
         * @brief 构造默认屏幕映射。
         *
         * P0 -> LT
         * P1 -> RT
         * P2 -> RB
         * P3 -> LB
         */
        ScreenOrder make_valid_order()
        {
            ScreenOrder order;

            order.physical_to_screen_ =
                {
                    0,
                    1,
                    2,
                    3};

            order.screen_order_tie_ = false;

            return order;
        }

        CornerConfig make_test_config()
        {
            CornerConfig config;

            return config;
        }

    } // namespace

    TEST(DetectionValidatorTest, ValidConvexQuadrilateral)
    {
        auto result =
            validateDetectionGeometry(
                make_valid_measurement(),
                make_valid_order(),
                cv::Size(640, 480),
                make_test_config());

        EXPECT_TRUE(result.valid_);

        EXPECT_TRUE(
            result.rejection_reason_.empty());
    }

    TEST(DetectionValidatorTest, NonFinitePointFails)
    {
        auto measurement =
            make_valid_measurement();

        measurement.physical_corners_[0] =
            cv::Point2d{
                std::numeric_limits<double>::quiet_NaN(),
                100.0};

        auto result =
            validateDetectionGeometry(
                measurement,
                make_valid_order(),
                cv::Size(640, 480),
                make_test_config());

        EXPECT_FALSE(result.valid_);

        EXPECT_NE(
            result.rejection_reason_.find("非有限"),
            std::string::npos);
    }

    TEST(DetectionValidatorTest, InvalidMappingOutOfRangeFails)
    {
        auto order =
            make_valid_order();

        order.physical_to_screen_[0] = 5;

        auto result =
            validateDetectionGeometry(
                make_valid_measurement(),
                order,
                cv::Size(640, 480),
                make_test_config());

        EXPECT_FALSE(result.valid_);

        EXPECT_NE(
            result.rejection_reason_.find("映射"),
            std::string::npos);
    }

    TEST(DetectionValidatorTest, DuplicateMappingFails)
    {
        auto order =
            make_valid_order();

        order.physical_to_screen_[1] = 0;

        auto result =
            validateDetectionGeometry(
                make_valid_measurement(),
                order,
                cv::Size(640, 480),
                make_test_config());

        EXPECT_FALSE(result.valid_);

        EXPECT_NE(
            result.rejection_reason_.find("映射"),
            std::string::npos);
    }

    TEST(DetectionValidatorTest, DegenerateEdgeFails)
    {
        auto measurement =
            make_valid_measurement();

        /*
         * P0 与 P1 重合。
         *
         * 第一条边长度为 0。
         */
        measurement.physical_corners_[1] =
            cv::Point2d{100.0, 100.0};

        auto result =
            validateDetectionGeometry(
                measurement,
                make_valid_order(),
                cv::Size(640, 480),
                make_test_config());

        EXPECT_FALSE(result.valid_);

        EXPECT_NE(
            result.rejection_reason_.find("退化"),
            std::string::npos);
    }

    TEST(DetectionValidatorTest, ConcaveQuadrilateralFails)
    {
        auto measurement =
            make_valid_measurement();

        /*
         * 构造凹四边形。
         */
        measurement.physical_corners_ =
            {
                cv::Point2d{100.0, 100.0},
                cv::Point2d{200.0, 100.0},
                cv::Point2d{150.0, 150.0},
                cv::Point2d{100.0, 200.0}};

        auto result =
            validateDetectionGeometry(
                measurement,
                make_valid_order(),
                cv::Size(640, 480),
                make_test_config());

        EXPECT_FALSE(result.valid_);

        EXPECT_NE(
            result.rejection_reason_.find("凸"),
            std::string::npos);
    }

    TEST(DetectionValidatorTest, SelfIntersectingQuadrilateralFails)
    {
        auto measurement =
            make_valid_measurement();

        /*
         * 蝴蝶结结构。
         *
         * 当前实现会先在凸性检查阶段拒绝。
         */
        measurement.physical_corners_ =
            {
                cv::Point2d{100.0, 100.0},
                cv::Point2d{200.0, 200.0},
                cv::Point2d{200.0, 100.0},
                cv::Point2d{100.0, 200.0}};

        auto result =
            validateDetectionGeometry(
                measurement,
                make_valid_order(),
                cv::Size(640, 480),
                make_test_config());

        EXPECT_FALSE(result.valid_);

        EXPECT_NE(
            result.rejection_reason_.find("凸"),
            std::string::npos);
    }

    TEST(DetectionValidatorTest, PointOutsideImageFails)
    {
        auto measurement =
            make_valid_measurement();

        /*
         * 超出原图：
         *
         * width = 640
         * height = 480
         */
        measurement.physical_corners_[2] =
            cv::Point2d{700.0, 200.0};

        auto result =
            validateDetectionGeometry(
                measurement,
                make_valid_order(),
                cv::Size(640, 480),
                make_test_config());

        EXPECT_FALSE(result.valid_);

        EXPECT_NE(
            result.rejection_reason_.find("范围"),
            std::string::npos);
    }

} // namespace mark