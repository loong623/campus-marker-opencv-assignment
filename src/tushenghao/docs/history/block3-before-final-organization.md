# Block3历史记录（整理前快照）

此页保存历史过程；当前状态以[block3.md](../block3.md)为准。旧接手计划已经撤回，历史命令和源码路径记录当时布局。

# 板块3验收记录：语义映射（WIP）

> 状态：WIP，未完成。分支 `feat/corner-semantics`，WIP 提交 `99bc72c`。
> 前半部分保留原WIP历史；当前以最后的2026-10-06三L观察修复记录为准。H已720/720 PASS，整体仍WIP。
> 本轮[适用边界与已知局限](../evidence/block3/05-L-observation-v5-v7/适用边界与已知局限.md)明确亮度下限、暗帧区间、未标注限制与系统保证。

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

---

## 2026-10-05 修复内容（追加，仍为 WIP）

执行依据为本次终稿§12及冻结3-2/接口总汇总。实际起点HEAD为 `25b6cbebb1afba6fad89a0daba088684f83ea4a3`，原分支保留；以上历史记录原样保留。**撤回旧接手计划中的“观测顶点直接当角 / P1投影 / 删拟合 / 帧1200出框即完成”**。旧记录的 `max_validation_residual_=10.0` 不代表实际提交配置；当前YAML旧值为5，均不能代替本次预算批准。

本次未提交：用户明确自行commit。保留用户 `.gitignore` 改动，不改冻结原文、参考合成器、视频、公共布局及原三L `geometry_matcher/validation/observation/utils` 算法和参数。

新增模块将原图观察、联合边拟合、证据复核、M/S补全和内部阶段拆开。原图取真实CHAIN_APPROX_NONE连续弧；模型投影只引导搜索，不产生输出点。新增预算缺省未配置，生产阶段明确NOT_READY。公共process保留稳定层门控、元数据和输入序列检查，不造track。测试框架依赖已移除，原测试覆盖目的保留为普通C++程序；旧黑图成功测试改为反证，95°伪平行改为真实近平行/反平行用例。

### D01–D25 处理对照

“实现”只指代码及有限测试，不代表本终稿§9阶段验收已完成。

| 编号 | 当前处理 | 依据/限制 |
|---|---|---|
| D01 | 实现：删除最近白块fallback，assignment严格ID/一对一 | resolver缺M/错M负例；M/S由独立补全模块提供 |
| D02 | 实现：PreparedFrame原图视图，原图重新分割 | 黑原图伪工作证据拒绝；原图位置断言 |
| D03 | 实现：整轮廓取连续弧，不裁ROI再造闭合边 | 拟合只用实际链；ROI/断弧完整负例仍需补齐 |
| D04 | 实现：指定两边联合方向/有限位置/凸转折约束 | 原图单边切断、普通矩形替M、粘连拒绝；更多内外边竞争未闭合 |
| D05 | 实现：CHAIN_APPROX_NONE且去重支持点 | 不修改旧工作图分割；最少点数下尾已批准10 |
| D06 | 实现：删除固定10/50工作px搜索ROI | 关联预算用原图px，实际值待闭合 |
| D07 | 实现：无向锐夹角、有限段延伸、交点到真实连接弧 | 真近平行/反平行测试；病态角门限仍待支持范围依据 |
| D08 | 实现：原图直接拟合，像素中心映射/scale有限校验 | 三工作尺度及各向异性resize回归；不是全仿射范围验收 |
| D09 | 实现：阶段使用原图size | 工作480且测试显式供应六片身份时原图右下角有效；不是480补全通过声明 |
| D10 | 实现：orientation消费唯一性，截断/竞争保留unknown | semantic与集成测试；正式H方向尚NOT_RUN |
| D11 | 实现：补全、定位、阶段均拒绝CLEARLY_INCOMPLETE | 固定不完整假设负例 |
| D12 | 实现：四角来源/支持/线/交点/误差重新核查 | 凸四点空证据、NaN/负误差、错标签、截断、伪交点等拒绝 |
| D13 | 实现：稳定来源ID、两支持弧、转折弧、拟合线/原始误差 | audit逐角序列化；C原始JSONL保留 |
| D14 | 实现：残差平局由稳定证据ID决定 | measurement重排仍选同一观测 |
| D15 | 实现：先全局E_min后64ε平局集合、字典序；finite防御 | 菱形与近能量平局/NaN测试 |
| D16 | 实现：公共NOT_READY、metadata、时间/id/reset/实例隔离 | 稳定层尚未实现，公共无Detection/track |
| D17 | 实现：阶段bbox由当前屏幕四点生成 | 集成断言，不填虚假confidence |
| D18 | 实现：唯一runDecodePipeline供审计/Detector共用 | 逐帧原因/hash/证据；正式全视频尚NOT_RUN |
| D19 | 实现：真实BGR正例、物理位置/只读/原图负例 | 不以finite代替位置，不以95°模拟平行 |
| D20 | 实现：公共测试检查返回status/metadata/空track | 不丢弃process返回值 |
| D21 | 实现：finite/范围/未知重复字段/新增预算往返；语义零门限≤ | 配置及回归测试；生产新预算仍未配置 |
| D22 | 实现窄补丁：原三L后校验/补M/S、保留竞争与截断 | fixture分支与资源边界通过；正式统计/资源策略待闭合 |
| D23 | 范围外：原三L分类/面积算法未动 | 若阻碍H/视频须交具体反例，当前未取得正式阶段反例 |
| D24 | 部分：模型已确认、六项统计已批准，预算表可追溯 | 关联/assignment/资源未批准，生产占位值不当依据 |
| D25 | 部分：删除GTest依赖；角边绑定仍是内部显式表 | 表与确认图纸一致，绑定配置化待补，未修改模型hash |

## 2026-10-05 验证结果

先红后绿记录：[01-red.md](../evidence/block3/01-contract/01-red.md)。起点12/12通过不能暴露的五个问题已复现为失败：非法图像异常、公共门控、近能量平局、非有限排序、零语义阈值。

最终构建/检查日志见 [02-validation.md](../evidence/block3/01-contract/02-validation.md)。依赖：GNU13.3.0、CMake3.28.3、OpenCV4.6.0、OpenSSL3.0.13；工具另用nlohmann_json3.11.3。当前15个CTest程序不等于§9.1所有场景已闭合，旧Block1/2部分assert在Release失效的限制公开保留。

C隔离实验720/720正确指定外边观测、结构失败0，统计216行；条件、公式、六项批准与剩余recipe限制见 [block3_budget.md](../block3_budget.md)，复用检查见 [block3_fixture_reuse.md](../block3_fixture_reuse.md)。不是正式阶段H。大产物在build目录，删除前须归档；统计MD/CSV在docs/evidence/block3。

视频hash核对通过，解码1676帧（0..1675）；仅审计1200的未配置smoke，输出NOT_READY和明确缺预算诊断。正式全视频、V召回/分段、N/原32负例、Q定位、baseline差异均 **NOT_RUN**，无可报告通过率。V/N/U/O数量尚未标注/确认，不以0作分母。

## 2026-10-05 遗留问题与验收状态

1. 六项统计获批，但关联/圆角剔除/支持条件、assignment正式C统计与资源压力仍未闭合；不能开启生产预算或正式H。
2. §9.1完整外边竞争、断弧、远交点与边界截断等固定负例仍需逐条核查补齐；模型绑定配置化待补。
3. H只有plan、未看结果；预算全集冻结后通过正式检测链路跑一次，并分别报告隔离/阶段，不给正式检测读真值。
4. V/N/U/O盲标注及Q须用户确认；原32annotations未在提供目录找到。尚不能完成§9.3/9.4。
5. 原三L逻辑范围外风险（D23）：M/S候选不能补救三L误分类/错仿射。触发条件为复杂轮廓/噪声下生成不可信父假设；是否实际阻塞尚待H与实拍反例。负责位置为原geometry_observation/matcher/validation，不擅改。
6. 原图完整轮廓按角重复提取；未测性能/资源上限，不声称实时性已达标。批准仿射/更小目标条件尚缺依据，旋转网格不替代其验收。

当前结论：**修复批次已实施，阶段验收受阻，仍为WIP**。全部必须项和review gate通过后才能改“Block3完成（阶段验收）”；公共process届时仍可因稳定层缺失保持NOT_READY。

## 2026-10-06 继续执行记录（截至三项修正待审）

用户授权继续H、全视频及关联/资源预算测量；V标注明确暂时跳过。此前六项批准保留，C-v4整组关联/assignment/资源候选随后获批并写入生产配置，保存 [冻结v1](../evidence/block3/frozen_detector_v1.yaml)。原三L代码/参数、模型坐标、公共接口布局仍未改，HEAD仍25b6cbe，未commit。

完成了C真实工作图M/S指标、原/工作双向关联、连接弧/跨度和资源压力测量，检测与测量共用 [assignment_match_metrics.hpp](../../lib/geometry/assignment_match_metrics.hpp)。单epsilon的顶点数规则在C圆角/栅格边界失败，改为固定五比例枚举，仍独立检查全边界与三L中位面积比。所有C失败历史保留，规则只从C确定。预算数据与审批过程见 [block3_budget.md](../block3_budget.md)。

冻结v1的第一次H：阶段71/720、隔离543/720。失败定位到直边末端被简化顶点拆碎，增加相邻两段的实际连续并集，所有阈值不变；补充弧终点ID与稳定编号、支持弧不重用和转折连接连续性复核。固定222点失败边界随 [回归数据](../../tests/data/rounded_split_boundary.txt) 保存，普通C++测试旧单段逻辑红、并集逻辑绿，证据 [03-rounded-red.txt](../evidence/block3/03-baseline-v1-v2/03-rounded-red.txt)。修复后同预算H-v2：隔离720/720，阶段121/720，错误有效几何/方向均0，仍未通过完整阶段门槛。

资源：补全在实际下一合法分支触发全局上限后结束batch，保留精确K语义；原先1000父假设×54组件约4475ms，现约2.650ms，输出≤64/扩展≤4096/所有截断不唯一。64分支decode约200ms，不能宣称硬实时。[resources-v4.md](../evidence/block3/03-baseline-v1-v2/resources-v4.md)及JSON保存8个固定压力场景。

两次已批准v1预算全视频均1676/1676条，最新代码 [摘要](../evidence/block3/03-baseline-v1-v2/video-v1-budget-latest-code.json) 对应 `build/block3-video-v2-approved-v1.jsonl`，Detection帧0、截断0，pipeline mean约1.220ms、max约5.719ms；包括1200的父变换、支持拒绝和原始诊断。视频hash不变。V按用户暂跳，N/U/O无确认标签、Q无人工真值；召回/负例误检/Q定位及原32负例不能报告通过。当前无封存baseline产物可作逐帧对比。

仍未闭合的关键项是**此前关联测量只用理想变换，未计实际三L父投影误差**。已补C-v6，720/720都有身份正确的原生产者父假设，原值不动，原始支持/边界不按待求上限过滤。真实生产者口径提出仅三项修正：边位置2→9原图px、对父投影方向1→3°、M/S边界3.5→9.5工作px；其余预算不动。依据 [C-v6-association.csv](../evidence/block3/02-calibration/C-v6-association.csv)，可review [候选v2](../evidence/block3/candidate_detector_v2.yaml)。候选v2完整C链路已720/720、错误有效输出0（[摘要](../evidence/block3/02-calibration/C-candidate-v2-pipeline.json)），但依终稿§12仍须用户批准数值修正后更新生产/冻结并跑最终H与视频。

相关代码及新增有限/非有限预算测试构建通过，两个Release目录均15/15；新增配置字段逐个NaN/Inf检查、候选/输出精确K与剩余尾部、真实原图边界、断边、矩形替M、粘连、跨帧不复用、远交点和转折断弧负例均已纳入。未把程序数15/15当作完整§9.1清单闭合；模型绑定配置化/更一般支持范围等限制保留。

当前结论仍是WIP：H隔离已通过，完整阶段尚未通过，三项修正review待回复。最终冻结配置的H与全视频重跑/独立核查/文档归档预计在批准后约15–25分钟；审批等待不计为批准。

## 2026-10-06 冻结v2与最终验证（预算审批已闭合，H仍FAIL）

三项修正已获用户批准，生产配置仅更新原图边位置9px、对实际父投影方向3°和M/S工作边界9.5px。冻结v1、候选及早期失败记录保留，最新配置为[冻结v2](../evidence/block3/frozen_detector_v2.yaml)。上述历史对照表中的待审/NOT_RUN不再代表当前数值/运行状态；D06/D21/D22/D24的关联、assignment及资源预算均已测量、批准、冻结。

正式H阶段689/720：无噪声138/144，噪声551/576；隔离720/720。错误有效几何/方向、屏幕排序、bbox及证据缺失均0，最大真值误差1.346291202px。未达到原冻结门槛，H退出2。31条保守empty全部归因：M固定简化规则拓扑不可用16条，旧三L验证拒绝全部父15条（generate6→validate0）。不能将隔离720/720当作完整阶段验收；没有用H调预算。

全视频完成1676/1676，Detection帧0、截断0；1200补全1分支后P0原图边证据拒绝。V暂跳、N/U/O无确认标签、Q无真值、原32子集及baseline未提供，召回/负例误检/人工定位未验证。公共process仍NOT_READY。资源8压力场景已归档且截断方向安全，64分支decode约200ms，不是硬实时通过。

两个Release构建目录CTest均15/15，配置检查及diff检查通过；三L保护文件/参数、模型hash、公共布局未改变；没有commit。完整[最终验证](../evidence/block3/03-baseline-v1-v2/04-frozen-v2-validation.md)、[分层报告](../evidence/block3/03-baseline-v1-v2/H-video-v2.md)、[全部失败诊断](../evidence/block3/03-baseline-v1-v2/H-v2-all-failure-diagnostics.jsonl)和hash已归档。D23现在已有范围外上游具体反例，必须review修复范围；M/S规则调整同样需review，当前预算维持冻结。D25绑定配置化/完整§9.1及一般支持范围仍遗留。整体仍WIP，不改称“Block3完成（阶段验收）”。

## 2026-10-06 视频0检测独立归因补记

用户要求区分预处理/阈值/pipeline问题，补全视频第一个阻断阶段统计：905无验证父、58有父但无完整M/S、713完整补全后原图四角拒绝；measurement及后续有效输出0。713帧第一失败角P0/P1/P2/P3分别591/83/34/5。新增只读block3_video_diagnose复用正式模块，对7帧逐项关闭门限，不改变生产参数。帧1200仅关闭max_corner_error后阶段产生1Detection；P0/P2/P3实测交点弧距约2.000/3.126/2.236px均超冻结1.5。帧1完整补全后M轮廓160点、epsilon1简化51顶点，指定第二外边没有可用候选，逐项关闭单门限仍不能通过。

因此仅修H的M/S拓扑与旧三L验证不能保证视频检测，前述下一步方案需要纳入原图连续边弧与实拍支持条件诊断。具体[根因报告](../evidence/block3/03-baseline-v1-v2/video-v2-root-cause.md)、[全视频分组](../evidence/block3/03-baseline-v1-v2/video-v2-stage-census.json)和7帧原始支持/指标已归档。不用诊断Detection代替真值精度，也未从视频调生产预算。Detector/旧三L代码和冻结配置未改，未commit。

## 2026-10-06 四项并行修复交付（H705，原689无回退）

用户批准完整修复、四项并行及实拍P99+1预算；同时要求模型/三L设计问题暂停报方案。当前生产为冻结v4，角上限4.131595359474918原图px，合成历史1.5保留。713组2852角全部逐个测量，弧扩展后2681可测/171保留null；其他门限不变。弧构造扩大真实连续区间并补既有trim内原始端点，帧1反例红→绿。M/S保留旧规则、增加细候选和有界拓扑，H原16条M反例全恢复，638视频恢复补全。

独立budget-only H689、arcs-only689、ms-only705、最终组合705：每项均逐ID保留原689，wrong0。最终C720/720；H无噪声141/144、噪声564/576、隔离720/720，仍未达正式门槛。全视频591/1676帧Detection、截断0；剩905无父、57未完整补全、123原图角拒绝。两个Release完整构建/CTest15/15、配置check及diff检查通过，资源8场景复测有界/截断方向安全。

905无父（非985）为325灰度≤150无白像素、257少3L、323残差全拒。H15实例mask确认真L被类M剔除、真M类L进错父，旧validator正确拒绝。按用户条件暂停三L类别/firstconcave设计修改；模型坐标无错误证据。58视频独立重放：18不完整裁切父、39边界超9.5、1纯拓扑修复，不宣称58全部恢复。V仍暂跳，未标注视频不给正确率，public仍NOT_READY。

完整[四项报告](../evidence/block3/04-four-track-v3-v4/06-four-track-repair.md)、[H分层](../evidence/block3/04-four-track-v3-v4/H-video-v4.md)、[防回退逐ID](../evidence/block3/04-four-track-v3-v4/four-track-H-nonregression.json)、[原始压缩归档](../evidence/block3/04-four-track-v3-v4/four-track-raw-archives.json)及hash已保存。原三L及模型/公共布局未改；没有commit。D23现为确定的分类/锚点适用设计暂停；D24新预算已批准执行，D25/完整§9.1及总体阶段仍WIP。

## 2026-10-06 用户批准三L观察层方案：实施与回归完成

按parent-v2-diagnosis.md四条实施：完整实测轮廓显式L多候选、候选及原简化轮廓全部凹点枚举、325暗帧不改亮度规则、固定规则并全H防回退和全视频回归。三L模型/仿射、残差5工作px、面积0.5–2及所有既批预算不变；来源ID查找和诊断传递修正，锚点尝试计入原1000资源上限。生产参数与[冻结v7](../evidence/block3/frozen_detector_v7.yaml)数值一致。

H阶段/隔离720/720 PASS：无噪声144/144、噪声576/576，原689和上轮705逐ID全部保留，错角/错方向/排序/bbox/证据差异0。C720/720，两个Release完整构建与CTest均16/16。模型hash/公共布局不变，未commit。

全视频1676/1676，Detection帧864；旧591保留581、新增283、10帧因新增竞争尚未排除保守拒绝，完整证据归档。剩余812互斥为325零白像素、239不足三L、24无完整补全、106原图角取证拒绝、118竞争未排除；截断0，锚点尝试最大48。三L有效父1112帧，旧771全部保留；这不是视频真值召回。

用户要求的[《适用边界与已知局限》](../evidence/block3/05-L-observation-v5-v7/适用边界与已知局限.md)已归档：严格gray>200、201只是必要像素下限；325暗帧五个区间及“未标注不判漏检”；最终失败类型/逐帧记录；系统保证的证据行为及不保证的亮度、通用检出率、空间位姿/硬实时/稳定track。V按用户暂跳，不报告未标注视频正确率或Q精度。

完整[实施报告](../evidence/block3/05-L-observation-v5-v7/07-L-observation-repair.md)、[H/video分层](../evidence/block3/05-L-observation-v5-v7/H-video-v7.md)、[防回退](../evidence/block3/05-L-observation-v5-v7/L-observation-H-nonregression.json)、[全部原始压缩与hash](../evidence/block3/05-L-observation-v5-v7/L-observation-raw-archives.json)保存，中间失败v5/v6不覆盖。此前“设计暂停/H失败”是历史；本轮授权范围与H门槛闭合，公共process因稳定层仍NOT_READY，D25绑定配置化/完整§9.1及未标注真值项保留，整体Block3仍WIP。

## 2026-10-06 批准后的目录重组

按依赖分析清单实施6个lib模块、include/mark及app，共48文件移动；fixture移到test_support，摘要头移到tools/common。源码仅include路径及app配置根层数调整，参数/模型/算法不变。主工程与独立工具全量构建通过、16/16测试通过；C/H各720与全视频1676条和搬迁前冻结v7逐条输出相同（忽略耗时/新代码hash），视频仍864检出，公共仍NOT_READY。见[目录重组记录](../architecture/layout-reorganization.md)与[迁移/回归验证](../architecture/layout-reorganization-verification.json)。没有commit，留用户review。
