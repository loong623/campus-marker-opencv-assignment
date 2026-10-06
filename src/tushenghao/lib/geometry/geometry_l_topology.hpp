// 批准的观察层L规则：固定多简化、完整原始弧检查、显式长臂L，输出仅实测点。
#pragma once
#include "core/geometry_types.hpp"
#include "mark/detector_config.hpp"
namespace mark {
std::vector<LTopologyCandidate> observeLTopologies(const WhiteComponent&,const GeometryConfig&);
}
