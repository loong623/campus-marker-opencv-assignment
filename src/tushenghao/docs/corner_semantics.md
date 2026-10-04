# Corner Semantics(Block 3)

## 1. 文档目的

本文定义 MARK 图纸模型中的四个物理角 P0~P3，以及它们与 MarkerGeometry 模型顶点、邻边之间的绑定关系。

本文只负责：
- 物理角身份定义
- 模型顶点绑定
- 模型邻边绑定

本文不负责：
- 图像检测角点
- 当前观测点计算
- 屏幕坐标排序
- 方向解析

---

## 2. 物理角定义

P0~P3 是目标自身固定编号，不是屏幕位置。

### P0：模型左上完整 L

- 模型左上完整 L
- 外侧上边与外侧左边交汇形成的凸角
- 对应：L0 外侧左边 + 外侧上边

### P1：模型右上分段角

- 模型右上 M 分段结构
- M 块外侧上边与外侧右边交汇形成的凸角

**特别说明，P1 不取：**
- S 中心
- 分段区域质心
- M/S 包围框角
- bbox corner

P1 必须来自 M 模型外侧上边和外侧右边的交汇凸角。

### P2：模型右下完整 L

- 模型右下完整 L
- 外侧右边与外侧下边交汇形成的凸角

### P3：模型左下完整 L

- 模型左下完整 L
- 外侧下边与外侧左边交汇形成的凸角

---

## 3. 边编号规则

模型 YAML 中只有 vertices 列表，没有单独保存 edge 编号。

边编号按顶点顺序推导：
- e0 = v0 → v1
- e1 = v1 → v2
- 依次类推
- 最后一条边返回 v0

来源：`config/marker_geometry.yaml` 中 vertices 数组下标即顶点编号。

---

## 4. Corner Binding Table

| Physical Corner | Model Piece | Vertex | Adjacent Edges | Source |
|---|---|---|---|---|
| P0 | L0 | v0 = vertices[0] | e5 = vertices[5]→[0]；e0 = vertices[0]→[1] | marker_geometry.yaml |
| P1 | M1 | v1 = vertices[1] | e0 = vertices[0]→[1]；e1 = vertices[1]→[2] | marker_geometry.yaml |
| P2 | L2 | v0 = vertices[0] | e5 = vertices[5]→[0]；e0 = vertices[0]→[1] | marker_geometry.yaml |
| P3 | L3 | v0 = vertices[0] | e5 = vertices[5]→[0]；e0 = vertices[0]→[1] | marker_geometry.yaml |

---

## 5. 后续接口边界

- **CornerResolution**：根据当前图像观测边，求当前角点，返回成功/失败状态
- **ScreenOrderResult**：根据 P0~P3 物理身份，建立屏幕顺序
- **DetectionValidation**：检查最终四点是否满足有效性

失败不用 `(0,0)` 这类特殊坐标表示，必须用明确的结果状态。




