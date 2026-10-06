# Final-fixes：Path A 锚点竞争修复——Codex 执行方案终稿

## 1. 任务与完成含义

在 `feat/final-fixes` 完成一次 **Path A 定点修复尝试**：阻止没有独立 L 结构依据的凹点生成伪几何竞争，保留有依据的全部竞争解释。本轮不承诺整个视频闪烁消失。

用户已批准范围。Codex只修改文件、构建、运行和归档，不执行任何 Git 命令，也不提交、切分支、push、merge。

| 范围 | 本轮行为 |
| --- | --- |
| Path A：基线 118 帧 UNRESOLVED_COMPETING_GEOMETRY | 修正锚点资格与来源证据，验证这 118 帧的变化 |
| Path B：角点支持弧、连接／延伸预算等 | **尚未实现／延期到 fix2**。可以记录，不修改 |
| 基线无几何假设的 564 帧 | **尚未实现／延期到 fix2**。不将它们默认标为真空帧或漏检帧 |
| 24 帧未完成 M/S 对应 | 不修，保持原算法；记录为范围外 |
| README 快速开始等 P1 改版 | 不在本轮；只补 Path A 状态、使用说明和文档链接 |
| 性能 | 记录，不优化、不改变搜索上限 |

“Path A 本轮实施通过”与“P0 闪烁整体解决”是两个结论。若代码／测试或效果验收失败，必须记录失败并停止扩大改动；可以提出后续问题假设，不能自行修 Path B 或其他问题。

## 2. 基线、冻结依据与已做反证

### 2.1 第 0 步必须核查的资料

- 分支：`feat/final-fixes`。
- 本方案审查快照：`e0b9d185d66440abe70aaebd885012ad1ed0a946`。
- 读取本地 `src/tushenghao/docs/ref/` 下 Block2、Block3 和接口冻结文档，特别是 `Block3_偏离审计与Codex修复方案_终稿.md` 的竞争处理要求。
- 读取现有 `docs/final-fixes_acceptance.md`、README、INDEX、相关源码与测试。
- 用户实际旧运行：`new-runs/verification/`，旧记录为 1676 帧，864 DETECTED、812 NOT_DETECTED，结果指纹 `c1785fb03a54ed25`。参数、输入文件必须与此运行核对。

Codex不能为核查执行 Git。可只读 `.git/HEAD`、refs／packed-refs；worktree 情况按 `.git` 指向的 gitdir／commondir 读取，不写这些文件。若本地分支不对、源码与下文目标明显不符、基线数据不完整或已有相关用户修改冲突，记录阻塞并停下，不覆盖用户工作。

本方案文件行号来自上述快照，实施时同时按函数名定位，不机械套用过时行号。

### 2.2 冻结要求

冻结修复终稿要求：**“若未解析项可能对应不同几何且无法排除 → empty”。**

本轮通过纠正上游假设的锚点资格修复问题，**不删除这条安全规则**。有结构依据的候选取证失败，仍属于可能未解的竞争；不能因为有另一个成功候选就跳过它。

### 2.3 已核实 bug

`lib/geometry/geometry_matcher.cpp:85–94` 的 `measuredAnchors()`：

1. 收集 `l_topology_candidates_` 中的实测凹角；
2. 无条件追加 `simplified_polygon_` 的所有凹转折；
3. 无有效结果时，还用 `anchor_vertex_index_` 回填。

错误是把“这个白片有 L 解释”扩大为“这个白片的每个凹转折都有 L 锚点资格”。噪声转折进入仿射拟合后可能通过宽一些的父验证，却在四角取证失败；decode 按冻结规则拒绝整帧。

第 2 帧的两个补全假设使用同一套六片对应：有依据的 L3 锚点 `(592,331)` 成功；额外 `(589,325)` 不属于已验证 L 拓扑凹角，生成失败竞争并阻断输出。这些是工作图坐标，**不得写进算法判据**。

### 2.4 全视频隔离实验：参考结果，不是正式验收

审查时只在“存在 L 拓扑候选”时禁止追加原简化凹点；没有改预算、decode、角点或稳定器。生产观察入口的 L 分类已有拓扑候选，因此这一实验支持本轮方向。

| 项目 | 结果 |
| --- | ---: |
| 输入／记录 | 1676 帧 |
| 旧 DETECTED／NOT_DETECTED | 864／812 |
| 实验 DETECTED／NOT_DETECTED | 976／700 |
| 原 Path A 118 帧恢复 | 112 |
| 原成功帧新增失检 | 0 |
| Path A 外状态变化 | 0 |
| 原成功帧 raw 四点改变 | 20 帧 |
| 上述改变中最大对应屏幕点位移 | 约 1.205078 原图 px |

剩余 6 帧：153、280、382、648、1108、1573；其中 382 仍记录竞争未排除。这只是实验结果，本轮**不得为追到 118/118 或 112 个恢复而改阈值**。

raw 四点变化帧：26、145、173、201、204、212、223、225、306、350、667、697、772、783、983、997、1083、1524、1525、1551。

该实验只核对状态与 raw 四点，不是全部公共字段／方向／证据验收。正式实现仍须走后文全部检查。1.205078 不是新增容差。

## 3. 架构决定：前置锚点资格，不改竞争拒绝

### 3.1 锚点准入规则

三 L 仿射拟合只允许使用 `observeLTopologies()` 生成、且自身通过原有显式 L 结构规则的候选凹角。

必须保留：

- 所有合法拓扑候选，不只取第一候选；
- 所有不同的合法凹角坐标；
- 同一坐标对应的全部拓扑来源；
- 原有非镜像仿射检查、父几何验证、M/S 补全、资源截断与语义安全规则。

禁止准入：

- 只凭 `supported_classes_` 包含 L；
- 只凭 `turns_` 标 CONCAVE；
- 只凭 `anchor_vertex_index_`；
- 一点／两点“拓扑候选”、凹点下标越界、非有限坐标或与结构验证不符的凹点；
- 按某个视频帧、坐标范围、帧号、面积特例放行。

### 3.2 不引入新的失败豁免

本轮 **不实现 VALID／REFUTED／UNRESOLVED 新状态机**，不修改 `decodeStage()` 的 `unresolved` 行为。上一轮讨论的这类设计不属于本轮实施清单。

伪锚点在生成几何假设之前被排除；合法假设的取证失败仍保守处理。相同 component assignment 不自动等于相同几何；不能依据“同六片”“坐标很近”“选最高分”删除合法竞争。

### 3.3 数据结构和装配

只增加 geometry 内部 helper 与类型，不修改 `Detection`、`FrameResult`、公开头文件／公共函数签名，不新增 lib 模块、不引入依赖。

几何证据继续放入已有 `GeometryHypothesis::evidence_` 字符串集合；统一 FrameRecord 已序列化此字段，不新建另一套生产逐帧日志，不改 schema version。

## 4. 文件级修改地图

下表所有路径相对于仓库根目录。

| 操作 | 完整仓库路径 | 具体职责 |
| --- | --- | --- |
| 新增 | `src/tushenghao/lib/geometry/geometry_anchor_evidence.hpp` | 内部锚点来源类型、收集签名 |
| 新增 | `src/tushenghao/lib/geometry/geometry_anchor_evidence.cpp` | 逐候选验证、稳定去重、保存来源；不拟合仿射、不读取视频 |
| 修改 | `src/tushenghao/lib/geometry/geometry_l_topology.hpp` | 声明内部结构验证 wrapper，原观察函数签名不变 |
| 修改 | `src/tushenghao/lib/geometry/geometry_l_topology.cpp` | 复用已有 `explicitL()`，提供 wrapper；原 epsilon 网格与观察规则不变 |
| 修改 | `src/tushenghao/lib/geometry/geometry_matcher.cpp` | 85–94 行替换锚点收集；约 418–443 行保留来源并写证据；其他算法不变 |
| 修改 | `src/tushenghao/tests/geometry_l_topology_test.cpp` | 修正原手造锚点 fixture，保留多候选和资源边界检查目的 |
| 新增 | `src/tushenghao/tests/geometry_anchor_evidence_test.cpp` | 本方案 A01–A09 |
| 新增 | `src/tushenghao/tests/path_a_competition_test.cpp` | 本方案 A10–A13，测生产阶段接口 |
| 新增 | `src/tushenghao/tools/validation/path_a_verify.cpp` | 原／新完整记录比较、118 帧清单、差异报告；不参与检测准入 |
| 修改 | `src/tushenghao/CMakeLists.txt` | 增加一个内部源码、两个 CTest、一个校验工具；保持现有 23 个 CTest |
| 新增 | `src/tushenghao/docs/path_a_fix_log.md` | 实施流程、决策、失败记录，见 §10 |
| 新增 | `src/tushenghao/docs/path_a_acceptance.md` | 本轮验收及延期项，见 §10 |
| 修改 | `src/tushenghao/docs/final-fixes_acceptance.md` | 追加 Gate1／Path A 结果，保留原 13 项历史 |
| 修改 | `src/tushenghao/docs/INDEX.md` | 登记两个专项文档及证据目录 |
| 修改 | `src/tushenghao/README.md` | 只追加简短 Path A 状态、测试／工具命令和专项文档链接；不做 P1 全文改版 |

### 禁止修改的生产文件

`lib/geometry/geometry_observation.cpp`、`geometry_validation.cpp`、`geometry_assignment_completion.cpp`，`lib/corners/` 全部文件，`lib/pipeline/decode_stage.cpp`、semantic／publication／temporal／display／诊断序列化实现，`lib/preprocess/`、`lib/config/`、`include/mark/`、`app/`。

`config/` 下所有配置与模型内容不改；`docs/ref/` 和已有 `docs/evidence/` 原件只读；v0.1 不改。除列出的测试和新增文件外，不整理／格式化其他文件。

若必须改白名单外生产文件才能通过，停下记录理由，本轮不自主越界。

## 5. 关键类型、签名和实现逻辑

### 5.1 geometry_l_topology.hpp/cpp

新增内部函数：

```cpp
bool is_valid_l_topology_candidate(const LTopologyCandidate& candidate);
```

具体流程：

1. 要求 polygon 恰好六点；所有坐标有限；epsilon 有限且严格大于 0。
2. 要求 `concave_vertex_indices_` 恰好一个元素，且下标合法。
3. 转为 Point2d，调用本文件已有 `explicitL()`，不复制第二套几何判断。
4. `explicitL()` 为真，且算出的唯一凹点下标等于候选声明下标，才返回 true。
5. 其他情况返回 false。这里的 epsilon 检查是来源元数据完整性检查，不新增数值门限。

**不改 explicitL() 的长臂／笔画比、凸凹判定或 observeLTopologies() 的枚举逻辑。** 输入是内部候选记录，其原始轮廓一致性已经由观察入口生成过程检查；不要在 helper 里凭空添加无法获得的原图验证。

### 5.2 geometry_anchor_evidence.hpp/cpp

```cpp
struct LAnchorTopologySupport {
    std::size_t topology_candidate_index_;
    std::size_t concave_vertex_index_;
};

struct ObservedLAnchor {
    cv::Point2f point_;
    std::size_t source_component_id_;
    std::vector<LAnchorTopologySupport> topology_supports_;
};

struct LAnchorCollection {
    std::vector<ObservedLAnchor> anchors_;
    std::size_t rejected_topology_candidates_{0};
};

LAnchorCollection collect_observed_l_anchors(
    const ShapeObservation& observation,
    std::size_t resolved_source_component_id);
```

流程：

- 按现有 `l_topology_candidates_` 顺序遍历；调用上面的结构 wrapper。
- 合格候选取其唯一凹角的**实际坐标**，不产生均值／投影／补点。
- 用 Point2f 精确坐标相等合并重复点，保持首次出现顺序；重复点的来源追加到 topology_supports_。
- resolved_source_component_id 使用 matcher 已解析的 assignment 来源 ID，不用 observation 容器下标冒充显式 ID。
- 不合法候选计入 rejected_topology_candidates_；不抛掉其他合法候选。
- 没有合格候选就返回空锚点；**不从 turns_ 或 anchor_vertex_index_ 回填**。
- helper 不读配置、不读图像、不读 frame_id，不添加搜索上限／经验值，不按坐标 epsilon 去重。

### 5.3 geometry_matcher.cpp

替换匿名 helper `measuredAnchors()`。允许将其删除并直接调用新 helper，不改变 `generateGeometryHypotheses()`、`fitAnchorTuple()` 或其他现有函数签名。

在已有 combination 内收集三份 `LAnchorCollection`，其 source ID 来自该 assignment。锚点为空则当前 combination 无法建立有依据的三 L 拟合，跳过，不编造替代锚点。

示意，非完整实现：

```text
对于每个原组合：
    为三个 observation 收集合格实测锚点与全部来源
    任一为空：记锚点资格不足，跳过该组合
    沿原递归顺序遍历三个锚点列表的笛卡尔积
    沿用原资源上限与“确有下一项才截断”的规则
    将三个 point_ 传给原 fitAnchorTuple
    沿用 validAffine 和 buildGeometryHypothesis
    追加三条锚点来源证据，保留原 observed concave anchors 文本
```

内部 `tuple` 可以保存 ObservedLAnchor 的引用／下标，调用原拟合前构造原有 `array<Point2f,3>`。不要让新类型穿透公共接口。

每个父假设追加以下证据格式，每个 assignment 一条：

```text
anchor_topology/v1 part=L0 component=5 x=... y=... supports=0:4,2:4
```

- part 来自组合，不写死 L0／L2／L3 的排列。
- component 为来源 ID。
- x/y 用 classic locale 与 `numeric_limits<float>::max_digits10`，保证 round-trip，不截断。
- supports 为 `候选下标:凹点下标`，保留全部来源，按原候选遍历顺序。
- 不把“证据字符串是否存在”当生产准入；实际准入由 helper 完成。
- 本格式只供既有 evidence 序列化与专项核查，不改变 FrameRecord 的字段结构。

沿用 `geometry/anchor_expansions=` 和 `geometry/invalid_affines=`；可新增累计非法候选／无合格锚点诊断，计数口径写入文档。不新增逐像素／逐区间刷屏日志，不把同一 observation 在多个组合重复访问的计数误叫“不同组件数量”。

### 5.4 不允许的替代实现

- 只保留第一个凹点；
- 无条件删掉所有失败假设；
- 取到一份测量立即结束 decode；
- 将 NO_VALID_ADJACENT_EDGES 当作“该竞争已被反证”；
- 把原 118 帧号写进生产分支；
- 调整 corner budget／max_hypothesis_count；
- 为避免 fixture 失败恢复原无依据 fallback；
- 新增置信度、卡尔曼、多目标或有效 ID 编码。

## 6. 现有测试的必要修正

`tests/geometry_l_topology_test.cpp` 当前 `anchors()` 用两点 polygon 和 turns 构造 L；`originalAnchors()` 又用一点 polygon 伪装拓扑，断言裸凹点必须被保留。它们不符合本轮“逐锚点结构资格”契约，必须改输入与预期，而不是删掉用例。

具体做法：

1. `anchors(model)` 为每个模型 L 构造两个完整六点长臂 L 候选，分别对应原两种不同实测锚点位置。可按模型 polygon 同比例变换和平移生成测试候选；每个候选附正 epsilon 和真实唯一凹点下标。
2. 这是一份**内部收集／枚举单测 fixture**，不是声称两候选一定来自同一真实轮廓。来源真实性由 roundedL／noisyL 和生产 observeShapes 集成用例覆盖；文档解释这一区别。
3. 保留 `allAnchors()` 的“不能只取第一候选、正确锚点元组必须存在”断言。
4. 原 `originalAnchors()` 用例保留入口，改为“裸凹点不能借 L 类别获得资格”：一份合法六点候选支持 good；另一个 raw CONCAVE 为 bad、没有拓扑支持；确认任何生成元组都不使用 bad，合法 good 不被丢掉。更新用例显示名与中文注释。
5. `anchorResource()` 保留两锚点×三个组件×六种排列＝48 个元组的设计，继续断言 K=48 不截断、K=47 有下一元组时截断、validator 传播标志。若模型实际组合数量与 48 不符，先核对 fixture，不修改生产资源上限。
6. 现有 roundedL、shortM、noisyL、stableIds、winding、brightness 用例不删、不降低断言。
7. `geometry_matcher_test.cpp` 已采用 observeShapes 的真实六点 fixture，原则上不修改；失败时先查本轮实现。

## 7. 专项测试：输入、预期、断言

两个新测试沿用现有主动 check／异常返回机制，Release 也执行；不用 GTest 或新增框架。

| 编号 | 输入 | 必须断言 |
| --- | --- | --- |
| A01 | 六点长臂 L 候选 | wrapper 为真，凹点坐标／下标正确 |
| A02 | 一点、两点、矩形、短臂 M、NaN／Inf、越界或错误凹点下标、非法 epsilon | wrapper 为假；helper 不产锚点；不崩溃 |
| A03 | 合法 L 候选＋未支持 raw 凹点，且 raw 在第一位置 | 只收集合法锚点，不因 raw 第一而取它 |
| A04 | L 类别与 anchor_vertex_index 均存在，但没有合法拓扑 | 锚点为空，不能 fallback；生成器没有基于该观察的父假设 |
| A05 | 两个不同合法锚点，第二个是预期元组 | 两者都保留；完整枚举包含第二个 |
| A06 | 多拓扑给同一个像素锚点 | 拟合坐标去重为一个，来源列表完整且稳定 |
| A07 | 非连续来源 ID、观察容器重排 | assignment 引用原 ID；helper 不把 ID 当数组下标；等价父集合相同，不要求诊断排序相同 |
| A08 | K 恰好穷尽及 K 后仍有下一元组 | 延续原精确截断语义；非法仿射仍计元组预算 |
| A09 | 带圆角／噪声的已有真实 L fixture 经 observeShapes | 能收集真实拓扑凹点；所有输出有合法候选来源；M 不变成 L |
| A10 | fixture::scene 走原提取／观察／生成／验证／M/S 补全／decode | 正常链路仍产生合法当前检测；三个锚点来源可回溯；检测四点证据仍满足原 validator |
| A11 | 完整合法父＋另一个有结构资格但角点取证失败的父，组成同一 GeometryBatch | decode 仍 NOT_DETECTED 且 UNRESOLVED_COMPETING_GEOMETRY；不得因 A 路修复豁免它 |
| A12 | 有效测量＋search_truncated；同几何异方向合法竞争 | 延续原语义规则：不强称方向唯一；不同几何冲突仍拒绝；逐项核对原 semantic 测试预期 |
| A13 | 相同 raw 图像重复输入，重置后再跑；平滑开／关 | raw 状态／四点／方向相同；状态不泄漏；当前 empty 不由历史框补齐 |

A11 使用 `fixture::scene()` 与既有合法六片对应建立基准父；失败父保留合法三 L 锚点，仅在测试侧构造导致必要角取证不足的 M 对应／观测条件。每个父的 component 引用仍合法且当前；不要用明显无效格式被别的入口挡掉，冒充“合法竞争失败”测试。冻结判据不因本轮改变。

A12 优先调用现有 resolveSemantics 测试 fixture，检查原语义分支，不改其生产实现。A13 用相同像素、递增合法 frame_id／timestamp，比较 raw，不把稳定 track 必须相同当错误断言。

CTest 总数：原 23 个全部保留，加两个可执行测试，**Release／Debug 各 25/25**；新程序内部子用例另列结果。

## 8. 专项校验工具与全视频回归

### 8.1 新工具命令契约

```bash
build/final-fixes-path-a-release/path_a_verify --baseline-run <旧运行目录> --candidate-run <新运行目录> --config <原verification配置> --expected-frames 1676 --report-dir <新报告目录>
```

所有参数必填，不接受未知／重复／缺值参数；报告目录不得覆盖现有证据。配置参数仅供验证现有预算和模型，不用于重跑或调参。

工具复用 `tools/common/frame_record_reader.hpp/cpp` 的受控 JSON 类型与原有配置加载器，链接 `mark_frame_record_reader` 和 `mark_detector_internal_headers`，不新增 JSON 依赖。

### 8.2 工具必须完成的检查

1. 两运行记录非空、frame_id 唯一且逐帧覆盖 0～1675，记录数都为 1676；缺帧／重复／JSON 错误非零退出。
2. 用原 observability_verify 的 `--check-run` 先核完整性；专项工具再次核输入／模型指纹一致、原图／工作尺寸和时间戳逐帧一致。effective_config 中生产算法配置相同，允许 run-dir、版本／源码标签这些运行溯源项不同；不忽略 threshold、几何、corner 或 temporal 参数。
3. 从**旧记录** details.decode.diagnostics 的 `UNRESOLVED_COMPETING_GEOMETRY` 标识构造 A 集合；记录帧号，必须为 118。不要按新结果定义 A，不拿所有旧空帧当 A。
4. 对比旧／新 raw status、detections（全部现有字段）、tracks、display，但把差异分开报告。track 可因恢复的新检测影响后续平滑历史，不要求修复后指纹或 tracks 与旧版相同。
5. 旧 raw 成功 864 帧不得新增失检；A 外 raw status 必须不变。若出现新失检或 A 外状态改变，输出 FAIL／停点，不能改归类隐藏问题。
6. old DETECTED→new DETECTED 的 raw 四点／orientation 变化逐项记录，不偷偷使用浮点容差抹掉；记录最大对应屏幕点距离，不据此新增生产预算。
7. 核查新记录 `details.generated.hypotheses`、`details.validated.hypotheses`、`details.completed.hypotheses`（实际 schema 没有 details.geometry）：每个父的三个 L assignment 必须各有一条 anchor_topology/v1 证据；component、part、候选索引、凹点索引、坐标必须回指 details.observations。调用原结构 wrapper 核对候选，不只检查字符串有无。显式缺少详情不能当作“没有违规”，必须报告证据缺失。
8. 按父三 L assignment 的原顺序，从配置模型取得 model anchors，从三条锚点证据取得 Point2f；调用与原 `fitAnchorTuple()` 相同的 `cv::estimateAffine2D` 默认参数重算 2×3 矩阵。当前同构建记录以 17 位有效数字写 double，工具读取后要求记录 affine 与重算值逐项精确相同；不新增像素阈值。跨构建比较仍由现有 observability_verify 负责。另报告 model anchor 的实际投影误差，不能声称 residual=0 就是真值误差=0。若矩阵精确复算不一致，报告该工具关卡失败并保留两矩阵，不自行改成宽松误差门槛。
9. 新成功帧必须 status/payload 自洽、四点有限且原图范围正确、orientation/marker_code 符合冻结行为。每个新成功帧的当前 CornerMeasurement／evidence 由现有 `orderScreenCorners`、`validateDetectionGeometry` 等实际生产调用验收；若统一记录不能无损重建这项，工具明确标成“需专项测试／证据复核”，不伪造复核 PASS。
10. 所有 unresolved 新帧独立列出；“有资格父取证失败”仍留未解，不帮生产删父。

工具输出至少包含：

- `path_a_report.json`：完整性、配置保护、证据来源、原成功退化、A 外状态变化、机器检查的 PASS／FAIL；
- `path_a_frames.csv`：118 帧逐项 old/new status、旧／新父数、measurement数、原因、恢复或仍失败；
- `raw_changes.csv`：全部 raw 差异，独立列 status／corners／orientation／其他字段；
- `history_changes.csv`：track／display 差异，与 raw 分开；
- `remaining_failures.csv`：A 中仍失败帧及现有诊断；
- `review_required.csv`：所有新增成功以及 raw 几何／方向改变帧的证据复核清单。

报告中的机器 PASS **不代替用户 review gate**。任何真值／方向唯一性人工项不能由工具自称已批准。

### 8.2.1 证据图导出子命令

同一个 path_a_verify 增加只读子命令，不另建工具：

```bash
build/final-fixes-path-a-release/path_a_verify render --candidate-run <新运行目录> --video "<原视频路径>" --review-list <review_required.csv> --output-dir <新的证据图目录>
```

规则：按原视频从 0 起实际解码取帧，不以播放器时间近似取帧。每个清单帧输出 `frame-N-original.png`、`frame-N-evidence.png`、`frame-N-evidence.json`，以及一份 `review_index.csv`。显示用的图注不得遮住目标。

绘制数据全部读取统一日志的 `details.decode.measurements`：每份 measurement 是四条已序列化 CornerEvidence 的数组，物理角来自 physical 字段，原图交点／support_arcs／turn_arc 按日志原值画。不同 measurement 独立编号／独立图层，不能把不同父的四角拼成一份证据；记录中没有测量时只输出原图与明确空证据说明，不补点。原图和 evidence JSON 保持原始像素／数字；展示图只是可视化，不用截图代替数值依据。

为 raw 几何／方向改变帧，同时链接旧记录及旧角点数值；图中的旧框明确标为对照，绝不回写生产结果。只有 render 子命令可以额外链接 `mark_offline` 以复用渲染接口；也可在本工具匿名 helper 中按统一 JSON 字段绘制已有支持弧，不添加任何检测判据。损坏 JSON、缺帧或引用错误必须非零退出。

测量包装重建时必须逐字段检查当前 CornerEvidence schema，缺字段／NaN／非法标签不能默认为零。调用现有 orderScreenCorners／validateDetectionGeometry 的正式核查与图像导出分开；“图看起来有框”不等于验收通过。

### 8.3 验收门槛

| 项目 | 本轮必须达到 | 不允许做什么 |
| --- | --- | --- |
| 锚点资格 | 每个生成父的三 L 锚点有合法拓扑来源；无 raw-only fallback | 不允许恢复 fallback 凑视频数字 |
| 工程测试 | Release／Debug 25/25，原 23 个及原关键子用例保留 | 不删测试换绿 |
| 竞争安全 | A11／A12 全通过；decode 原规则不变 | 不跳过合法失败竞争 |
| 全视频完整性 | 两构建都完成 1676 帧，所有检查失败正确非零退出 | 不接受空比较／缺帧当 PASS |
| 既有成功保持 | 864 个旧 raw 成功帧零新增失检 | 失败则停，不改阈值 |
| 范围保护 | A 外 raw status 零变化；禁止文件／预算零变化 | 不自行修其他漏检 |
| 明确代表帧 | 2、1045 恢复且四点／方向证据合法 | 不只断言 detections≥1 |
| 完整效果表 | 118 帧全部分类；新成功与 raw 变化逐帧归档 | 不把 118/118 当硬追数任务 |
| 文档 | §10 全部完成、INDEX 登记 | 文档缺失不写“本轮完成” |

112 恢复、976 总成功是本轮隔离实验的**复现参考**，不是调参目标。正式实现若与它不同，必须解释具体帧差异；不能自动宣布同等效果或改算法追数。较少恢复／发生额外变化，记录为“待复核／效果验收受阻”，提交用户 review 后决定，不擅自加范围。

所有新增输出必须满足原四点／方向证据契约。原成功帧角点变化也必须复核；不要求旧错误的候选选择永久固定，但不能未记录便更换金样。旧指纹与旧运行原件永久保留。

## 9. 八步执行顺序与验证命令

命令在仓库根目录 Linux／WSL 终端执行，路径带空格时保留引号。输出与证据目录必须是新目录，不覆盖用户 `new-runs/verification/`。

### Step 0：核基线并建立只读旧证据

1. 只读核分支／基线和已有用户修改，检查 §4 文件白名单。
2. 核原视频实际可读、1676 帧；输入路径由用户现有配置／文件定位，不写进生产代码。
3. 将旧 verification 的 manifest、effective_config、frames、summary、run_export、export_timings 复制进 `docs/evidence/final-fixes/path-a/baseline/`；复制后比 SHA256，保留原件。
4. 把计划和基线清单写入 path_a_fix_log.md，首次状态 `实施中`。
5. 原 23 测试先分别运行，存在基线失败就如实记录，不能记成本轮通过；禁止替换损坏基线或伪造记录。

固定构建目录：`build/final-fixes-path-a-release/`、`build/final-fixes-path-a-debug/`。

```bash
cmake -S src/tushenghao -B build/final-fixes-path-a-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/final-fixes-path-a-release -j4
ctest --test-dir build/final-fixes-path-a-release --output-on-failure
cmake -S src/tushenghao -B build/final-fixes-path-a-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/final-fixes-path-a-debug -j4
ctest --test-dir build/final-fixes-path-a-debug --output-on-failure
```

产出：原 23 测试日志、输入／配置／模型哈希、旧 118 帧清单。若缺旧完整记录，停在此步，先补基线，不开始调参。

### Step 1：增加结构 wrapper 和锚点 helper

按 §5.1／5.2 落位。先写 A01–A09，再实现。中文注释写明“原来 L 类别被错误借给所有凹点，为什么逐候选验证且禁止 fallback”。

产出：helper 源／头、单测、CMake 库与测试目标登记。

验证：只跑 geometry_anchor_evidence_test；所有非法输入安全拒绝，所有合法多候选保留。

### Step 2：接入 matcher，保留原搜索语义

按 §5.3 替换 measuredAnchors 与递归 tuple 使用，追加来源证据。不要改原组合、affine 拟合、父验证、M/S 对应或资源上限。

产出：matcher 修改、锚点 evidence 与诊断说明。

验证：新 helper 单测、geometry_matcher_test；比对白名单与配置哈希。

### Step 3：修正内部手造 fixture，补竞争安全测试

按 §6 保留旧用例的有效检查目的，新增 A10–A13，两个构建全部跑 25 个 CTest。

```bash
ctest --test-dir build/final-fixes-path-a-release --output-on-failure
ctest --test-dir build/final-fixes-path-a-debug --output-on-failure
```

产出：25/25 日志和子用例结果。失败只在本轮白名单内排查；涉及禁止文件则停下记录。

### Step 4：先跑代表原图，后开发专项 verifier

用原视频解码第 2、1045、1039、1500、1573 帧：2／1045 为预期恢复例，后三帧是不得顺带改预算的观察例，不承诺一定恢复。

不能按 frame_id 触发生产特殊行为。测试侧取帧可以；输入 Detector 时使用合法递增帧号／时间。检查恢复例每个角的当前帧证据、原图坐标、屏幕顺序及方向唯一性。

实现 §8 的 path_a_verify，补空比较／缺帧／非法证据测试或工具负例验证，不另开第三个 CTest目标。不得调用工具“自动修正”数据。

产出：代表帧原图、证据图、工具负例结果。

新增工具的负例至少覆盖：两边都是空记录、缺一帧、重复 frame_id、没有详情却声称锚点合格、伪造 support 候选下标、配置预算被改、旧成功变空、A 外状态改变。全部必须非零退出；report-dir／output-dir 已存在时拒绝覆盖。可以合入上述两个新测试程序或采用留存命令验证，不能删这些负例以获得25/25。

### Step 5：全视频 Release／Debug 双回归

```bash
build/final-fixes-path-a-release/marker_app --check-config --config src/tushenghao/config/detector_verification.yaml
build/final-fixes-path-a-release/marker_app --video "<原视频路径>" --config src/tushenghao/config/detector_verification.yaml --mode debug --run-purpose verification --expected-frames 1676 --run-dir src/tushenghao/docs/evidence/final-fixes/path-a/verification-release
build/final-fixes-path-a-debug/marker_app --video "<原视频路径>" --config src/tushenghao/config/detector_verification.yaml --mode debug --run-purpose verification --expected-frames 1676 --run-dir src/tushenghao/docs/evidence/final-fixes/path-a/verification-debug
build/final-fixes-path-a-release/observability_verify --check-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-release --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/path-a/check-release.json
build/final-fixes-path-a-debug/observability_verify --check-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-debug --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/path-a/check-debug.json
build/final-fixes-path-a-release/path_a_verify --baseline-run src/tushenghao/docs/evidence/final-fixes/path-a/baseline --candidate-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-release --config src/tushenghao/config/detector_verification.yaml --expected-frames 1676 --report-dir src/tushenghao/docs/evidence/final-fixes/path-a/report-release
build/final-fixes-path-a-debug/path_a_verify --baseline-run src/tushenghao/docs/evidence/final-fixes/path-a/baseline --candidate-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-debug --config src/tushenghao/config/detector_verification.yaml --expected-frames 1676 --report-dir src/tushenghao/docs/evidence/final-fixes/path-a/report-debug
build/final-fixes-path-a-release/observability_verify --compare src/tushenghao/docs/evidence/final-fixes/path-a/verification-release src/tushenghao/docs/evidence/final-fixes/path-a/verification-debug --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/path-a/release-debug-compare.json
```

旧版／新版不做“零差异”金样检查，因为本轮就是修旧漏检；新版 Release／Debug 公共结果要求一致。耗时／源码溯源自然不同，不作为算法结果差异。

### Step 6：证据复核、失败分析，严守范围

1. 为所有 A 中恢复帧和所有原成功 raw 改变帧导出原图及当前角点／支持弧／方向来源的局部证据图；通过现有审计接口复用结果，不重新发明定位判据。
2. 新增成功与变化帧形成 reviewer 可逐帧核对的索引，图像不写在 build，指向永久证据目录。
3. 对所有残留 A 失败写清阻断阶段；Path B 原因只标“尚未实现／fix2”。
4. 若未通过硬门槛，停止追加生产修改。最多列 3～5 个后续问题点，每条写“观测、文件／函数、假设、最小验证实验、为什么不属本轮”；未验证必须标假设。
5. 不得在日志之后“顺手修几个”范围外问题，不得宣布整个 P0 消失。

```bash
build/final-fixes-path-a-release/path_a_verify render --candidate-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-release --video "<原视频路径>" --review-list src/tushenghao/docs/evidence/final-fixes/path-a/report-release/review_required.csv --output-dir src/tushenghao/docs/evidence/final-fixes/path-a/frames
```

### Step 7：文档、归档和清理

完成 §10 全部文档，核本地链接；列所有新／修改文件与禁止文件的哈希保护结果。测试工具仍是正常源码交付，不能把源码只留在 build。

归档构建／CTest日志、命令与返回码、输入哈希、统一运行记录、专项 CSV／JSON、原图／证据图、剩余帧列表后，确认文件可读、引用指向归档目录。

最后仅清理本轮两个固定 build 子目录及本轮产生且已归档的临时数据。先验证解析后的绝对路径处于仓库 build 下且精确匹配上述两个名字；不删除整个 build，不删用户 new-runs、不删既有归档、不删其他旧 build。测试受阻时也先归档失败日志再清理。

本轮产物不留对 build 的永久文档链接。将清理结果写入 fix_log；用户之后可按本文命令重建。

## 10. 中文文档交付：两份新增，三份同步

### 10.1 新增 `src/tushenghao/docs/path_a_fix_log.md`

施工日志，必须含：

1. 任务边界与本轮批准：只 Path A；明确 Path B／564 帧尚未实现、延期 fix2。
2. 基线、输入／配置／模型哈希、旧 118 帧集合来源。
3. Step0–7 执行表：开始／结束、实际耗时、操作、产出、验证命令／退出码、剩余项。
4. 根因与实施决定：逐锚点资格、保留全部合法解释、decode 不改；为何去掉 raw fallback。
5. 修改文件清单，以及原 fixture 改写的理由、保留的检查目的，不能一句“测试适配”。
6. 失败／排障记录：观察→验证→结论；成功时也写本轮未做什么。
7. 剩余问题与 3～5 个后续调查建议；若没有新建议写“未提出新的已验证问题”，不编造。
8. 归档、链接核查、固定 build 清理结果。

每完成一步立即补日志，不等最后凭记忆编过程；耗时记录实际值，不写预估冒充实测。

### 10.2 新增 `src/tushenghao/docs/path_a_acceptance.md`

验收报告，格式保留“完成内容／验证方法／踩坑记录／已知限制”四段式，并包含：

- 顶部状态：`实施通过，待用户 review`、`验收受阻` 或 `未完成`；**用户尚未 review 时不写最终批准完成**。
- 旧 118、恢复、残留、旧成功退化、A 外变化、raw 变化、方向变化、Release／Debug 差异表。
- 原 23＋新增 2 的测试结果，主动子用例列表；所有命令和证据相对链接。
- 流程正确性、检测效果和整体 P0 三个分开的结论。
- 原／新指纹及意义：新指纹不能抹掉旧指纹，差异不能自动叫回归。
- 当前帧角点／方向证据的复核结果与未审批清单。
- 限制：仍然可能闪烁；Path B 与 564 帧**尚未实现／延期到 fix2**；不做性能专项。
- 若失败，写最先失败关卡、影响、已留证据与建议，不要求为了写“完成”扩大修改。

### 10.3 同步已有三份文档

| 文档 | 本轮增补 |
| --- | --- |
| `docs/final-fixes_acceptance.md` | 追加“Gate1／Path A 定点修复”节及专项链接；保留原 13 项报告；明确原工程通过不代表闪烁已解决 |
| `docs/INDEX.md` | 登记两个专项文档、path-a 证据目录与读者推荐顺序 |
| `README.md` | 简短状态＋重建／专项核查命令＋两个链接；不得借此混入 P1 全文改版 |

不再新增零散设计稿／debug-log 副本。机器 JSON／CSV、图像和命令日志全部放 `src/tushenghao/docs/evidence/final-fixes/path-a/`。

建议证据树：

```text
docs/evidence/final-fixes/path-a/
├── baseline/                 # 旧运行只读副本与哈希
├── tests/release/            # CTest和构建日志
├── tests/debug/
├── verification-release/    # 统一FrameRecord及运行报告
├── verification-debug/
├── report-release/           # 专项JSON/CSV
├── report-debug/
├── frames/                   # 当前原图、角点证据图、review索引
├── failures/                 # 若失败，最小复现与日志
├── commands.md               # 实际命令、返回码、环境
└── hashes.json               # 输入/配置/模型/保护文件及产物哈希
```

若目录已有本轮证据，不覆盖；在父目录下建立明确命名的 attempt-02，文档指出哪次是最新，旧尝试保留。

## 11. 最终停止条件与交付回复

遇到以下任一情况，立即停在本轮范围内，先归档，再提交说明：

- 基线或分支不符、输入或原记录不完整；
- 需要改冻结文本、数值预算或禁止生产文件；
- 旧成功帧新增漏检、A 外状态改变；
- 必须删除合法竞争才有恢复；
- 新输出无法满足原四点／方向证据契约；
- 25/25 未通过；
- 112 个恢复的参考结果无法解释性复现；
- 出现其他问题，只能本轮之外修改才能解决。

失败也要交完整的日志和受阻验收报告；最多给出 3～5 个下一轮问题点，不实施它们。

最后回复：本轮改了什么、测试与 118 帧效果、哪些仍未解决、用户 review 要看的文件与帧号、build 清理情况。明确 **“Path A 定点修复尝试的结果”**，不能写“全视频漏检已解决”。

## 12. 可直接贴给 Codex 的执行提示词

```text
任务：按《Final-fixes：Path A 锚点竞争修复——Codex 执行方案终稿》执行一次限定范围修复。

先做Step0核基线：feat/final-fixes，本方案审查快照e0b9d185d66440abe70aaebd885012ad1ed0a946。
只读核分支和源文件，不执行任何Git命令。保护用户已有修改。

本轮只修Path A：三L锚点必须有独立合法L拓扑依据。不得让supported_classes、raw CONCAVE、anchor_vertex_index替代逐锚点资格。
使用现有explicitL规则，新增geometry内部helper保留所有合法锚点与全部拓扑来源。
接入原matcher递归；保留组合、仿射拟合、资源截断、父验证、M/S补全。
decode_stage的unresolved规则不改，有资格候选取证失败仍保守拒绝。

禁止改Path B的corners/preprocess/预算，禁止修无假设564帧；这两项在文档写“尚未实现/延期到fix2”。
不得改公开接口或Detection/FrameResult布局，不改config、模型、冻结ref原文、旧evidence，不做性能优化、置信度或历史框补点。
不得执行git，不能删测试换绿，不能拿detections>=1或视频输出文件存在作为完成标准。

严格按本文白名单、签名、伪代码、A01-A13与Step0-Step7执行。
所有实质改动有清晰中文注释：原来什么问题、为什么这样改。
现有geometry_l_topology_test的手造两点/一点假拓扑必须按本文修正输入及资格预期，保留多候选、资源边界与真实噪声fixture的检查目的。
Release/Debug原23测试保留，新增两个CTest，总计25/25。

原118帧集合由旧frames.jsonl定义。独立实验参考是恢复112、总976成功，无旧成功新增失检、A外状态不变，但有20帧raw角点改变；这不是调参指标，也不是正式实现已经过审。
正式实现重跑1676帧双构建，专项工具核资格来源、逐帧差异、原864成功保持、A外状态不变、合法竞争安全。
所有新增成功、raw/方向变化须有当前帧证据和review索引；不能自动覆盖旧金样。

两份新中文文档：docs/path_a_fix_log.md、docs/path_a_acceptance.md。
同步docs/final-fixes_acceptance.md、docs/INDEX.md、README中的本轮状态和链接；不做P1全文改版。
所有证据归档docs/evidence/final-fixes/path-a/，固定build/final-fixes-path-a-release和-debug。
归档并检查链接后，仅清理本轮两个build目录及已归档的本轮临时数据，不清整个build、不动用户new-runs和旧证据。

若任何硬关卡失败，停止扩大修改，写受阻记录；可提交最多3-5个后续问题点，标注证据/假设/最小实验，本轮不实施。
最后报告Path A定点尝试结果、回归数据、剩余帧、review入口和清理结果；未经用户review不能写最终批准完成，更不能写整个闪烁已解决。
```
