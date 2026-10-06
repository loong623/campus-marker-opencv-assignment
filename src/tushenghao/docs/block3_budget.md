# Block3 预算与审批表（2026-10-05，WIP）

> 当前进度（2026-10-06三L观察修复后）：用户批准观察多候选/全凹点方案，冻结v7全部数值与v4一致，max_corner_error仍4.131595359474918原图px；合成1.5px与实拍分布并列保留。
> C/H阶段均720/720，原H689与705无回退；视频864/1676，旧581保留/10竞争保守回退/283新增。H阶段门槛PASS，整体WIP。
> V暂跳，视频召回/Q未验证。下文早期待审和暂停记录为历史，最新见[观察层修复](evidence/block3/05-L-observation-v5-v7/07-L-observation-repair.md)与[适用边界](evidence/block3/05-L-observation-v5-v7/适用边界与已知局限.md)。

依据：终稿 §5；模型批准记录来自本会话用户确认：`config/marker_geometry.yaml` 已按图纸核对，M=160、L=416、S=64 可用。
模型SHA-256：`844853098082c0eae1972f744e084ca8b928233bf62b6c3933c6af4ada374109`。
后续用户已批准下表六项统计候选；关联规则、assignment和资源预算仍未批准。生产 `detector.yaml` 未改数值；新增预算节点缺省为显式未配置。

## 固定实验与实测状态

C-v1：1440×1080 BGR、16px名义笔画；0～345°步长15°；无/2px相切圆角；无噪声及σ=1灰度噪声seed11/22/33/44；中心(720,540)。240张原图分别通过480×360、960×720、1440×1080真实preprocess，共720次隔离定位。

已运行一次C隔离实验：720/720得到正确指定外边的观测测量，结构失败0；工具用实例真值供应assignment与确定变换，并核验支持点对应指定模型边。正式Detector不读真值。待求残差/交点/延伸/方向上限没有被应用；每角每线原始值和支持像素都保留。

**仍是实验recipe v1，不是正式批准预算。** 简化、位置关联、圆角剔除、连接弧、跨度和病态规则使用显式实验值，尚无批准记录。它们须独立完成依据/审批；不能因720成功就将recipe偷偷写入生产配置。H只有plan（未渲染、未定位）；没有看H误差定值。

| 统计候选 | 单位/含义 | C分层最终候选 | 审批 |
|---|---|---:|---|
| max_corner_error | 交点到实测转折连接弧距离，原图px | 1.5 | 已于本会话2026-10-05批准 |
| max_edge_direction_diff | 拟合边对投影模型边无向锐夹角，度 | 1 | 已于本会话2026-10-05批准 |
| max_support_extension | 交点超出有限支持段的近端延伸，原图px | 9.5 | 已于本会话2026-10-05批准 |
| max_line_fit_error | 每线平均垂距，原图px | 0.5 | 已于本会话2026-10-05批准 |
| min_line_points | 每样例八边去重支持点最少值，下尾 | 10 | 已于本会话2026-10-05批准 |
| 合成/人工真值定位预算 | 相对连续名义物理角欧氏误差，原图px | 2.0 | 已于本会话2026-10-05批准；不供运行Detector读取真值 |

算法逐样例先求四角/八边最坏值，分工作尺度×圆角×噪声×L/M统计；标准差n−1，nearest-rank P99/P01。上限max(μ+3s,P99)，px向上0.5、度向上1；点数min(μ−3s,P01)，向下整数且≥3。最后取各层最大上限/最小下限。

每层n、失败数、μ、s、分位数、最大/最小值、公式原值、取整、最坏case：
[evidence/block3/C-v1-summary.csv](evidence/block3/02-calibration/C-v1-summary.csv)。摘要：[C-v1-summary.md](evidence/block3/02-calibration/C-v1-summary.md)。原始证据：`build/block3-C-measure-v1.jsonl`；输入/真值：`build/block3-C-v1/`。摘要及CSV应随代码保存，较大图像/原始证据在本地构建目录；删除build前须归档。

本会话用户明确选择：“批准六项统计候选，其余仍待审”。只记录六项批准；预算全集未齐，新增observation/assignment节点尚未启用，生产数值待一致落地。

## 未闭合的关联/assignment/资源预算

| 字段 | 当前状态 | 必需工作 |
|---|---|---|
| approximation_epsilon（原图px） | recipe=1，未批准 | 独立说明转折/直弧划分依据；避免用自身门限截断再求统计 |
| turn_trim_distance_px | recipe=2，未批准 | 证明圆角连接弧剔除规则和支持跨度，不仅说明已知圆角半径 |
| max_edge_position_distance_px | recipe=4，未批准 | 独立测指定模型边关联的位置误差；不能继承recipe作为生产预算 |
| max_component_mapping_distance_px | recipe=5，未批准 | 测原/工作轮廓双向关联误差，尤其最小工作尺度 |
| max_turn_connection_length_px | recipe=12，未批准 | 连接弧长度定义已实现；需独立依据/测量 |
| min_support_span_px | recipe=5，未批准 | 支持跨度下尾及可辨条件；不能用均值+3s定下限 |
| min_intersection_angle_deg | recipe=5，未批准 | 已批准仿射范围/条件数依据，不能从正例90°平均数猜门限 |
| assignment边界/简化/方向/相对面积 | fixture专用值，仅验证分支行为 | 正式原工作图测量报告与审批；不复用旧三L0.5～2门限 |
| assignment有限边界采样步长 | fixture=1工作图px，未批准 | 核查连续边界距离覆盖/采样误差；未宣称连续全空间保证 |
| assignment候选/扩展/输出分支上限 | 只有显式测试资源值，未批准 | 用户资源策略+边界/压力实测，不能套3σ或正例平均数 |
| 仿射/更小目标支持条件 | 没有独立批准范围资料 | 当前C仅旋转与等比尺度；不冒称全仿射/远距离覆盖 |

这些不是缺模型，也不是再次请求G1/G2/G5许可。未闭合项属于原终稿保留的G3/G4与资源review gate；不能把生产阶段NOT_READY改成“就绪但无目标”。

## 未执行与下一关卡

- H：NOT_RUN（plan为240原图/720定位，seed55/66/77/88，中心1000.25/750.5；未读取检测结果）。
- assignment正式C测量：NOT_RUN；当前通过的是有限契约fixture。
- 具体覆盖缺口：480×360固定fixture在未批准的assignment测试预算 `{boundary=3, epsilon=1, direction=10°, relative_area_error=0.08, sample_step=1}` 下无补全候选、expansions=0。测试保留保守拒绝，不以放宽测试值消除此反例；正式C需测清轮廓/相对面积/拓扑误差。该例不是原三L算法失败证据，因测试侧已给三L身份/变换。
- 全视频正式阶段回归：NOT_RUN；只作预算缺失状态的单帧1200工具smoke。
- V/N/U/O盲标签及Q：未完成/未确认；原32annotations未在已提供目录找到。
- 六项统计候选已批，但不能跳过上述规则/资源关卡与H/视频验收；本页不表示Block3完成。

## 2026-10-06 关联与资源候选（新测量，等待review）

用户本轮要求继续H、全视频和预算测量，明确暂时跳过V标注。因此V/N/U/O/Q相关召回/误检真值结论暂不作为本轮已闭合项，不能把无标注视频的Detection当正确率。

关联原始测量复用同一C-v1输入，720条全保留；版本v2/v3用于暴露和核查assignment拓扑规则，v4采用固定五比例简化（上限3工作px，1/3、1/2、2/3、5/6、1倍）。原始边界/面积不受误差上限过滤。v2单epsilon1工作px结构失败315条（另3条分层不足记录），v3五个固定单值分别仍有失败；联合固定枚举后v4定位/assignment结构失败0。没有读H结果选规则。

正式补全与工具共用 `assignment_match_metrics.hpp`：三L观测面积中位数为基准、有限边界双向采样、凹凸绕向/循环对应，匹配指标由所有拓扑可用简化候选的最小方向差定义。1px采样的距离为1-Lipschitz，连续边界上界再加0.5px，明确列出这项覆盖裕量。原三L算法参数不动。

逐层统计：[C-v4-association.md](evidence/block3/02-calibration/C-v4-association.md)、[CSV](evidence/block3/02-calibration/C-v4-association.csv)。原始：`build/block3-C-measure-v4.jsonl`。

| 待批字段 | 候选 | 来源/条件 |
|---|---:|---|
| 原图edge position | 2.0px | C-v4上尾、0.5px取整；实际支持到正确有限模型边 |
| 原/工作component mapping | 4.5原图px | C-v4上尾4.0，加独立采样覆盖0.5；raw最大3.605551 |
| turn connection length | 13.5原图px | C-v4上尾；C全部成功、连接弧原始像素链保留 |
| min support span | 14.0原图px | C-v4下尾，向下0.5；只覆盖本轮16px名义笔画 |
| 原图approximation epsilon | 1.0px | 固定recipe分段规则；保留真实链而非简化边拟合 |
| turn trim | 2.0原图px | 固定相切圆角半径2的recipe规则；须与上项共同审批 |
| min intersection angle | 5° | 明确保守病态拒绝策略，非3sigma；C/H支持旋转/等比尺度，非全仿射证明 |
| assignment boundary | 3.5工作px | C-v4统计3.0，加采样覆盖0.5 |
| assignment direction | 30° | C-v4方向上尾，整数取整；小S/M栅格边较短，不等于定位两外边1°预算 |
| assignment relative area | 0.045 | C-v4上尾；无量纲0.005网格；raw最大0.04045954 |
| assignment epsilon cap | 3.0工作px | 固定五比例枚举，在C闭合拓扑，非真值择优 |
| assignment sample step | 1.0工作px | 显式有限段采样及0.5px覆盖裕量 |
| candidates / expansions / output | 128 / 4096 / 64 | 每父每缺失片候选扫描上限、全batch扩展上限、全batch输出上限；资源策略，不套统计公式 |

六项既批指标保持原值。新数值未写生产YAML，待review后一起冻结。当前支持条件是固定C/H的1440×1080、16px名义笔画、等比旋转、0/2px圆角和σ1噪声；视频真实尺度/仿射覆盖未经V确认，不能自动宣称覆盖。

资源压力固定6场景（每M/S复制0/1/4/16/64/256个合法组件、6～774总组件），输出上限64，扩展上限4096，候选每片128；达到上限但尚有竞争均标truncated，且阶段不宣称唯一。实测补全约0.33～12.96ms，进程累计峰值RSS约24.5MiB；这是本机单次补全模块压力测量，不是全pipeline实时性能承诺。原始 `build/block3-resources-v2.jsonl`；精确K与剩余尾部分别有普通C++回归断言。

候选配置由强类型导出为新文件，审批前仅用于C诊断，不是正式H/生产。H图像已生成，检测结果暂未读取。

## 2026-10-06 批准与后续生产者口径修正

用户本轮已明确“批准整组候选，继续冻结与验收”；上述v4规则/数值写入生产detector.yaml，冻结快照 [frozen_detector_v1.yaml](evidence/block3/frozen_detector_v1.yaml)。这项批准不表示H/视频通过。

v1正式H：阶段71/720，隔离543/720，错有效几何/方向0；原始 `build/block3-H-verify-v1.jsonl`。圆角组失败来自单个简化段把连续直边末端拆碎，不是放宽阈值能合理解决。新增相邻两段连续并集候选、保留全部像素/普通拟合/独立支持约束后，v2在同一冻结预算下隔离720/720；阶段121/720，错有效输出0。H失败原记录保留，不删除分母。

**此前关联预算漏计了真实三L生产者的投影偏差。** v4使用真值身份+理想变换，其位置/方向统计不能直接覆盖消费真实三L变换的正式链路。该测量范围遗漏现已补正，未改原三L代码或参数，也未用H/视频反推数值。

C-v6再次使用同一固定C输入，以实例真值仅在工具侧识别正确三L父assignment；原生产者 `generateGeometryHypotheses→validateGeometryBatch` 的变换原值不动。720/720存在身份正确且非镜像的父假设。计算实际父投影模型边到原图真实支持、实际父投影M/S到真实工作组件的原始指标，不应用待求上限。先按样例最坏值再按工作尺寸/圆角/噪声分层，原统计公式不变；结构失败0。

| 请求修正的生产字段 | 已批v1值 | C-v6候选 | 依据 |
|---|---:|---:|---|
| observation.max_edge_position_distance_px | 2.0原图px | 9.0原图px | 真实三L父投影位置上尾9.0；raw最大8.62117779 |
| observation.max_edge_direction_diff_deg | 1° | 3° | 真实三L父投影方向上尾3°；raw最大2.15889603° |
| assignment.max_boundary_distance_work_px | 3.5工作px | 9.5工作px | 真实父投影M/S双向边界上尾9.0，加1px采样覆盖0.5 |

其余已批准数值保持不变（包括线残差0.5px、交点连接弧误差1.5px、延伸9.5px、最少10点、真值2px）。此前理想变换隔离方向1°仍是该口径的实测事实，但生产字段消费的是实际父投影，必须明确review其3°修正，不能把两种口径混写。修改搜索关联边界不会取消当前原图支持、普通拟合、凸转折、残差/有限段/真值验收约束。

统计见 [C-v6-association.md](evidence/block3/02-calibration/C-v6-association.md) 与 [CSV](evidence/block3/02-calibration/C-v6-association.csv)，原始 `build/block3-C-measure-v6.jsonl`；仅这三项等待新review，未改冻结v1/生产值。C-v5原图拟合统计仍在原批准值内，无需重批其余六项。

## 2026-10-06 最终数值批准与冻结v2实测

用户明确回复“批准这三项修正，继续验收”：原图边位置9px、拟合边对真实三L父投影方向差3°、assignment边界9.5工作px。仅这三项从v1改变，其余统计、关联、规则、采样及128/4096/64资源预算保留。方向1°仍是理想变换C测量历史，不是当前生产上限。

生产[detector.yaml](../config/detector.yaml)与[冻结v2](evidence/block3/frozen_detector_v2.yaml)已启用；冻结v1及所有候选/失败历史保留。数值审批、关联测量与资源策略已闭合，不能因此宣称H验收通过。正式H689/720，31失败有完整逐父/候选原始指标，未用于放宽预算；全视频1676记录、0Detection。见[最终验证及范围冲突](evidence/block3/03-baseline-v1-v2/04-frozen-v2-validation.md)。

## 2026-10-06 用户批准实拍角预算重定与M/S构造修复

本轮用户明确批准按713组实测P99+1原图px重定max_corner_error；此授权优先于终稿旧“禁止从视频定值”约束，其他数值门限不变。统计固定原713完整补全帧和旧六片父assignment/仿射，共2852角全部逐个尝试，不能因resolver首角失败跳过其余角。

| 来源/条件 | n/可测 | P0 P99 | P1 P99 | P2 P99 | P3 P99 | max_corner_error |
|---|---|---:|---:|---:|---:|---:|
| 合成C-v1、16px笔画/0或2px圆角/σ1；原批准统计法 | 原720样例 | 历史分层表 | 历史分层表 | 历史分层表 | 历史分层表 | 1.5原图px（历史） |
| 原713实拍组、旧弧候选、仅解角误差上限 | 2852/1484 | 3.188164 | 2.458908 | 3.381701 | 2.828428 | 不采用不完整旧候选分布 |
| 同713实拍组、扩大真实连续弧；其他预算不变 | 2852/2681 | 3.101948 | 2.814812 | 3.131595 | 2.882911 | max四角P99+1=4.131595359474918（当前） |

平均/std/P50/P95/P99/最大值、每物理角713分母、171不可测原因及前后输入SHA全部见[完整统计](evidence/block3/04-four-track-v3-v4/video-713-corner-budget-v3.md)及JSON。误差是拟合交点到实际转折弧的距离，不是人工真值误差；人工定位2px预算没有随该生产上限扩大。不可测保存null，不混为0误差，不宣称全部2852成功。

M/S规则保留旧五比例，增加固定1/4细候选与最多两额外实测顶点有界省略（完整原弧偏离仍≤原epsilon3px）；新构造获本轮完整修复授权，没有改变boundary9.5/方向30/area.045/资源128-4096-64。[独立反例与58帧](evidence/block3/04-four-track-v3-v4/ms-topology-validation-repair.md)。

生产和[最终冻结v4](evidence/block3/frozen_detector_v4.yaml)已同步；[四项独立验证](evidence/block3/04-four-track-v3-v4/06-four-track-repair.md)保留budget-only/arcs-only/ms-only/组合四个H对照，原689均无回退。H705仍因15条上游真实身份排除失败，禁止通过改validator或门槛掩盖；三L设计暂停并交方案。

## 2026-10-06 已批准三L观察方案执行

固定显式L多候选与全实测凹点规则见[实施报告](evidence/block3/05-L-observation-v5-v7/07-L-observation-repair.md)。未重定白阈值/残差/面积、未增加观察闭合误差门槛，数值保持v4。原1000上限现在计入包括无效/镜像的全部锚点元组，精确K与截断主动检查通过；最终H720/720，视频锚点尝试最大48、截断0。适用下限与未标注限制见[边界文档](evidence/block3/05-L-observation-v5-v7/适用边界与已知局限.md)。
