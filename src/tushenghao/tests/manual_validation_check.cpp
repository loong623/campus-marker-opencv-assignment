// TODO: 赛前用合成器做完整验证后删除
/* 本次简单验证逻辑：
1. 用完美 mock 数据（3 个标准 L，位置都对）跑 Step 6，得到 6 个假设
2. 喂给 Step 7 验证
3. 算对的标准：validated 非空（至少留下一个）
这个测试只验"不误杀"，没验"能抓错"。要验抓错，得喂错数据（比如把 affine 搞歪）
下一步：
近的（可选）：造一组错数据（比如把某个 observation 的坐标故意挪歪 20 像素），看 Step 7 能不能把它拦下来。这是验"抓错"能力。
远的（必须）：Step 8，把 5→6→7 串进 detector.cpp，让它真正跑起来。（detector 集成：不是做新算法，而是把已经冻结的 Block 2 模块接入 Detector 主流程。）
现在不做，Step 9 全回归时一起补。
*/
// mock 测试:直接造假数据调 validateGeometryBatch，只验逻辑对不对，不用等合成器。
// 赛前调参（Step 4 的完整测量）:合成器做精细调参,生成带真值的图像，跑完整管线（适合调参数）。
#include "geometry_matcher.hpp"
#include "geometry_validation.hpp"
#include "marker_geometry.hpp"
#include "detector_config.hpp"

#include <iostream>

namespace
{

    mark::ShapeObservation createLObservation(std::size_t id_offset)
    {
        mark::ShapeObservation observation;
        observation.simplified_polygon_ = {
            {0.0f + static_cast<float>(id_offset), 0.0f},
            {20.0f + static_cast<float>(id_offset), 0.0f},
            {20.0f + static_cast<float>(id_offset), 10.0f},
            {10.0f + static_cast<float>(id_offset), 10.0f},
            {10.0f + static_cast<float>(id_offset), 20.0f},
            {0.0f + static_cast<float>(id_offset), 20.0f}};
        mark::TurnFeature turn;
        turn.vertex_index_ = 3;
        turn.type_ = mark::TurnType::CONCAVE;
        observation.turns_.push_back(turn);
        observation.supported_classes_.push_back("L");
        observation.anchor_vertex_index_ = 3;
        return observation;
    }

    mark::MarkerGeometry createSimpleGeometry()
    {
        mark::MarkerGeometry geometry;
        geometry.schema_version = 1;
        for (const auto &id : {"L0", "L1", "L2"})
        {
            mark::GeometryPolygon polygon;
            polygon.id = id;
            polygon.vertices = {
                {0.0f, 0.0f}, {20.0f, 0.0f}, {20.0f, 10.0f}, {10.0f, 10.0f}, {10.0f, 20.0f}, {0.0f, 20.0f}};
            polygon.anchor = polygon.vertices[3];
            polygon.area = 300.0;
            geometry.polygons.push_back(polygon);
        }
        return geometry;
    }

    mark::WhiteComponent createWhiteComponent(std::size_t id_offset)
    {
        mark::WhiteComponent comp;
        comp.area_ = 300.0;
        int o = static_cast<int>(id_offset);
        comp.contour_ = {{o, 0}, {o + 20, 0}, {o + 20, 10}, {o + 10, 10}, {o + 10, 20}, {o, 20}};
        comp.touches_border_ = false;
        return comp;
    }

} // namespace

int main()
{
    mark::GeometryConfig config;
    config.max_hypothesis_count_ = 100;

    std::vector<mark::ShapeObservation> observations;
    observations.push_back(createLObservation(0));
    observations.push_back(createLObservation(30));
    observations.push_back(createLObservation(60));

    std::vector<mark::WhiteComponent> components;
    components.push_back(createWhiteComponent(0));
    components.push_back(createWhiteComponent(30));
    components.push_back(createWhiteComponent(60));

    mark::MarkerGeometry geometry = createSimpleGeometry();

    auto batch = mark::generateGeometryHypotheses(observations, geometry, config);
    std::cout << "Step6 hypotheses: " << batch.hypotheses_.size() << std::endl;

    auto validated = mark::validateGeometryBatch(batch, geometry, components, config);
    std::cout << "Step7 validated: " << validated.hypotheses_.size() << std::endl;

    for (const auto &d : validated.diagnostics_)
    {
        std::cout << "  diag: " << d << std::endl;
    }

    if (validated.hypotheses_.empty())
    {
        std::cout << "FAIL: all rejected" << std::endl;
        return 1;
    }
    std::cout << "PASS" << std::endl;
    return 0;
}
