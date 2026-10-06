#include "geometry/geometry_matcher.hpp"
#include "geometry/geometry_observation.hpp"
#include "observability_fixture.hpp"
#include <iostream>
#include <map>
#include <set>
using temporal_fixture::check;
namespace {
// 原 fixture 的锚点共线、模型身份错误且没有真实拓扑；答案仅用于测试侧筛选和事后核对。
void real_three_l() {
    auto scene = fixture::scene();
    auto config = observability_fixture::app().detector_config.geometry_;
    auto model = fixture::model();
    std::map<std::string, std::size_t> truth;
    for (const auto &a : scene.hypothesis.assignments_)
        if (a.model_part_id_ == "L0" || a.model_part_id_ == "L2" || a.model_part_id_ == "L3")
            truth.emplace(a.model_part_id_, a.component_id_);
    std::vector<mark::WhiteComponent> components;
    for (const auto &c : scene.frame.components_)
        for (const auto &a : truth)
            if (c.component_id_ == a.second)
                components.push_back(c);
    check(components.size() == 3, "three actual L components missing");
    auto observations = mark::observeShapes(components, config);
    auto batch = mark::generateGeometryHypotheses(observations, model, config);
    check(!batch.hypotheses_.empty(), "real nondegenerate three-L fixture rejected");
    bool found = false;
    for (const auto &h : batch.hypotheses_) {
        check(h.assignments_.size() == 3, "three assignments required");
        std::set<std::size_t> used;
        std::map<std::string, std::size_t> mapping;
        for (const auto &a : h.assignments_) {
            check(used.insert(a.component_id_).second, "duplicate component assignment");
            check(std::any_of(components.begin(), components.end(),
                              [&](const auto &c) { return c.component_id_ == a.component_id_; }),
                  "missing assigned component");
            mapping.emplace(a.model_part_id_, a.component_id_);
        }
        check(h.affine_transform_.rows == 2 && h.affine_transform_.cols == 3 &&
                  cv::checkRange(h.affine_transform_),
              "finite affine required");
        found = found || mapping == truth;
    }
    check(found, "true model correspondence absent");
    auto limited = config;
    limited.max_hypothesis_count_ = 1;
    check(mark::generateGeometryHypotheses(observations, model, limited).resource_truncated_,
          "resource truncation flag lost");
    // 保留每个合法 L 的像素轮廓，只将实测凹锚点平移到同一直线上。
    for (std::size_t i = 0; i < components.size(); ++i) {
        auto it = std::find_if(observations.begin(), observations.end(), [&](const auto &o) {
            return o.source_component_id_ == components[i].component_id_;
        });
        check(it != observations.end() && it->anchor_vertex_index_, "real anchor missing");
        auto anchor = it->simplified_polygon_.at(*it->anchor_vertex_index_);
        cv::Point delta(cvRound(200 + 200 * i - anchor.x), cvRound(300 - anchor.y));
        for (auto &point : components[i].contour_)
            point += delta;
        components[i].bounding_box_ = cv::boundingRect(components[i].contour_);
    }
    auto degenerate =
        mark::generateGeometryHypotheses(mark::observeShapes(components, config), model, config);
    check(degenerate.hypotheses_.empty(), "collinear anchors accepted");
}
} // namespace
int main() { return temporal_fixture::run({{"actual_model_three_l", real_three_l}}); }
