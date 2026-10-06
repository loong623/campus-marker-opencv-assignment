// 验证 Step 6 跑得通：喂三个 L 观测进去，能吐出带仿射矩阵的几何假设，不崩。
/* Step 6 假设生成链:
ShapeObservation
    +
MarkerGeometry
    +
GeometryConfig
          |
          v
generateGeometryHypotheses()
          |
          v
GeometryBatch
*/
#include "geometry/geometry_matcher.hpp"
#include "core/marker_geometry.hpp"
#include "mark/detector_config.hpp"

#include <cassert>
#include <iostream>

namespace
{

    // 构造一个简单 L 型 ShapeObservation。
    // 注意：这里不是 detector 输出 ID。
    // supported_classes_ 只是 Step5 给出的形状候选。
    mark::ShapeObservation createLObservation(
        std::size_t id_offset)
    {
        mark::ShapeObservation observation;

        // 一个简单 L 形轮廓
        observation.simplified_polygon_ =
            {
                {0.0f + static_cast<float>(id_offset), 0.0f},
                {20.0f + static_cast<float>(id_offset), 0.0f},
                {20.0f + static_cast<float>(id_offset), 10.0f},
                {10.0f + static_cast<float>(id_offset), 10.0f},
                {10.0f + static_cast<float>(id_offset), 20.0f},
                {0.0f + static_cast<float>(id_offset), 20.0f}};

        // 模拟 Step5 已经发现凹点。
        // Step6 只能使用这个几何证据，
        // 不知道它对应哪个 MARK ID。
        mark::TurnFeature turn;

        turn.vertex_index_ = 3;
        turn.type_ = mark::TurnType::CONCAVE;

        observation.turns_.push_back(turn);

        // 形状候选。
        // 这里的 L 不是已知 ID，
        // 只是 ShapeObservation 的类别支持。
        observation.supported_classes_
            .push_back("L");

        observation.anchor_vertex_index_ = 3;

        return observation;
    }

    mark::MarkerGeometry createSimpleGeometry()
    {
        mark::MarkerGeometry geometry;

        geometry.schema_version = 1;

        // 三个模型 L 组件。
        // 注意：
        // 这些是模型几何接口提供的信息，
        // 不是 detector 输入的答案。
        for (const auto &id :
             {"L0", "L1", "L2"})
        {
            mark::GeometryPolygon polygon;

            polygon.id = id;

            polygon.vertices =
                {
                    {0.0f, 0.0f},
                    {20.0f, 0.0f},
                    {20.0f, 10.0f},
                    {10.0f, 10.0f},
                    {10.0f, 20.0f},
                    {0.0f, 20.0f}};

            polygon.anchor =
                polygon.vertices[3];

            polygon.area = 300.0;

            geometry.polygons.push_back(
                polygon);
        }

        return geometry;
    }

}

int main()
{
    mark::GeometryConfig config;

    config.max_hypothesis_count_ = 100;

    std::vector<mark::ShapeObservation>
        observations;

    // 三个白块观测。
    // Step6 从三个观测推理 MARK 解释。
    observations.push_back(
        createLObservation(0));

    observations.push_back(
        createLObservation(30));

    observations.push_back(
        createLObservation(60));

    mark::MarkerGeometry geometry =
        createSimpleGeometry();

    mark::GeometryBatch batch =
        mark::generateGeometryHypotheses(
            observations,
            geometry,
            config);

    // 至少应该生成一个三 L 假设。
    assert(
        !batch.hypotheses_.empty());

    // 每个假设必须包含三个 assignment。
    for (const auto &hypothesis :
         batch.hypotheses_)
    {
        assert(
            hypothesis.assignments_.size() == 3);

        // 必须存在仿射矩阵。
        assert(
            !hypothesis.affine_transform_
                 .empty());
    }

    // 验证资源限制字段存在并可使用。
    if (batch.resource_truncated_)
    {
        std::cout
            << "resource limit triggered\n";
    }

    std::cout
        << "geometry_matcher_test passed\n";

    return 0;
}