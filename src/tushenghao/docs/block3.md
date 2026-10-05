# 板块3验收记录：语义映射（WIP）

> 状态：WIP，未完成。分支 `feat/corner-semantics`，WIP 提交 `99bc72c`。
> 本文档记录 Block 3 当前技术现状，供 Codex 接手与后人查阅。

## 完成内容

- Block 3 集成链路接入 `Detector::process()`（`src/tushenghao/detector.cpp`）：
  `GeometryHypothesis → resolveObservedCorners → CornerMeasurement → resolveSemantics → orderScreenCorners → validateDetectionGeometry → Detection`
- `find_component()` 增加 M1 fallback：hypothesis 无 M1 assignment 时，用 affine 投影模型 M1 anchor 找最近白块（`src/tushenghao/corner_resolver.cpp`）
- Block 2 顺带修复：
  - `polygonResidual()` 改 `cv::pointPolygonTest()` 几何距离（原下标对比无对应关系）
  - `observeShapes()` L 分类 `turn_count==4` → `6||7` + 凹点检查（原误把四边形标为 L）
  - `manual_validation_check` mock 几何修复

## 验证方法

- CTest 12/12 通过
- 端到端：`./build/tushenghao/decode_audit data/raw/marker_video.avi 1200` → `hypotheses: 2`，`detections: 0`

## 踩坑记录

1. `polygonResidual()` 下标对比问题 → pointPolygonTest
2. L 分类误标四边形 → 6/7 顶点 + 凹点
3. P1/M1 不在 hypothesis assignment 里 → affine 投影 fallback
4. `resolveObservedCorners()` 边拟合 `find_matching_edge()` 找错边（近平行），调阈值无效——属结构问题，非参数问题

## 已知限制

- 端到端未闭环，`resolveObservedCorners()` 为主要风险点
- 待验证：P0/P2/P3 能否直接用观测 polygon 顶点；P1 是否必须 fallback；边拟合去留
- `max_validation_residual_=10.0` 等阈值未经系统验证
- `FrameResult::diagnostics` 未正式填充（Step 7 缺口，Block 5 处理）

## Codex 接手计划

- 任务：重构 `resolveObservedCorners()`，P0/P2/P3 直接用观测顶点，P1 用 affine 投影，删边拟合
- 约束：中文注释、函数签名不变、只改 `corner_resolver.cpp`、12/12 通过、`detections ≥ 1`
- 交付标准：帧 1200 `detections ≥ 1`