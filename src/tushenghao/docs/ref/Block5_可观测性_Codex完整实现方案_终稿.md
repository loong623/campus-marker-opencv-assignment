# Block5：可观测性——Codex 完整实现方案终稿

日期：2026-10-06。执行分支：`feat/observability`。起点：`b9cccd4a86ac5959a8c67559cde1d399ba29e046`。

本文件是实施方案，不是实施完成报告。用户批准本方案所列范围；Codex 编写和验证，用户负责 Git 与最终 review gate。未运行的检查不得填 PASS。

## 1. 依据、已批准决策及范围

### 1.1 依据

| 文件/记录 | 地位 |
|---|---|
| `src/tushenghao/docs/ref/5.md` | Block5 计时、诊断、模式、审计分工、验收的冻结依据 |
| `src/tushenghao/docs/ref/4.md` | 稳定层公式、三历史、失检及显示隔离依据 |
| `src/tushenghao/docs/ref/v0.8.1_Detector.md` | 公共契约及总体边界 |
| `src/tushenghao/docs/ref/Block4_输出稳定层_Codex完整实现方案_终稿.md` | Block4 后续批准范围、接口与验收模板 |
| Block4 前置深审稿 | 本轮 D01：发布精度冲突；D02：重置原因丢失；编号不对应 Block3 的 D01–D25 |
| `src/tushenghao/docs/evidence/block4/` | 已归档的批准、测试、视频和预算证据；保持只读 |

本提案实际核对了当前分支源码、冻结文件、归档目录、`G-B-approval.json` 和 `approved-comparison.json`。本提案没有重跑 Linux C++ 测试/视频；此前 WSL 返回 E_ACCESSDENIED。执行 Codex 必须自行获取运行证据。

### 1.2 用户已批准，直接执行

| 决策 | 本期动作 |
|---|---|
| D01 Route B | Step 0 统一 Block3 发布 float 规范顺序，单独验收；允许相应公开点序/orientation 置换，不允许移动物理点或改阈值 |
| D02 | 统一事件框架保留 reset 原因，不改公共调用接口 |
| 四项装配要求 | 实现 FrameRecord 内部通道、阶段插点、App 离线 baseline/debug、诊断开销记录 |
| geometry_audit | 新建，复用现有几何流程，不添加算法判据 |
| 六模块 | 继续 core/config/preprocess/geometry/corners/pipeline；本方案不需要第七模块 |
| diagnostics 兼容 | `FrameResult.diagnostics` 保留 `std::vector<std::string>`，不删除、不替换 |
| G-ABI/G-C/G-B | 保留 DisplayState.value；本期不做置信度；批准预算 r=2px、偏离=2px不调整 |
| 证据位置 | 所有本 Block 证据归档 `src/tushenghao/docs/evidence/block5/`；固定 build 目录，核验后清理 |

目标：真实回答各阶段耗时、真实拒绝/重置原因、显示来源，给出可复现完整算法基线。只观察；除已批准 Route B 外不改变检测、解码、关联、平滑及准入规则。

禁止把性能未达14ms当作授权优化；14ms@1440×1080是用户系统目标，对照 `process_total`，不是作业下限、不是性能保证。

## 2. Step 0：基线确认与 Route B 前置补丁

### 2.1 开工确认：不执行 Git

直接读取 `.git/HEAD`、refs；若 `.git` 是 worktree 指针，解析实际 gitdir/commondir，兼容 packed-refs。确认工作分支为 `feat/observability`、起点完整 hash 如本文。分支或 hash 不符先停止并交用户；不 checkout/reset/pull/fetch/commit/push，也不由 CMake 间接调用 Git。

将源文件列表、SHA256、基线与参数审批依据写 `docs/evidence/block5/step0/baseline.json`。先建立固定构建目录的任务标识文件；已有目录无本任务标识时不删除、不覆盖，报告用户。

执行前建立原始基线构建并保留：19 个既有 CTest、全视频0–1675的结果；同时核对 Block4 归档日志及输入 SHA256。视频位置通过参数传入，不写死 `/home/...`。预期输入 hash：`aa1219a7a7b702ea1265be8752853a267c982a651afa0f7846f516f9517c9ac7`。hash 不同不能冒称同视频回归。

### 2.2 修改地图（行号对应 b9cccd4）

| 完整路径 | 函数/位置 | 动作 |
|---|---|---|
| `src/tushenghao/lib/pipeline/decode_stage.cpp` | `decodeStage()` 第52–68行发布循环 | 替换 double 排序后直接 float 发布段，调用新发布 helper |
| `src/tushenghao/lib/corners/detection_publication.hpp/.cpp` | 新 `publishFloatDetection()` | 所有生产 raw Detection 的 float 排序及发布检查只在此处完成 |
| `src/tushenghao/tests/block3_contract_regression_test.cpp` | 新增发布边界用例，不删除旧项 | 测忠实于生产顺序的分界、置换、退化和范围检查 |
| `src/tushenghao/tests/temporal_integration_test.cpp` | 增加 raw 接收断言 | 发布 helper 的产物进入 update，不能因重排而 INVALID_INPUT |
| `src/tushenghao/CMakeLists.txt` | mark_detector 源列表 | 注册发布 helper；Step 0 暂不增加 CTest 程序数量 |

保持 `orderScreenCorners()` 的 P0–P3 double 契约、`orderScreenCycle()` 的能量/容差/字典序规则、CornerEvidence 内容、所有公共签名不变。Step 0 不放宽 `validRaw()` 检查。

```cpp
std::optional<Detection> publishFloatDetection(
    const CornerMeasurement& measurement,
    bool orientation_unique,
    cv::Size original_size,
    const CornerConfig& config,
    std::string& rejection_reason);
```

前提：该 measurement 已经通过现有 double 排序及 `validateDetectionGeometry()`；helper 不承担重新取证或语义消歧。

**关键逻辑（示意，非实现）：**

```text
P0–P3 原始 double 角 → 各自转 float → 再转 double 表达这些 float 值
检查 float 发布值 finite、画内、严格凸、非重复/非退化
对这个仍按物理编号排列的环调用既有 orderScreenCycle
公开 corners = 规范 float 屏幕序；bbox = float 点 min/max（不加1）
若本帧方向唯一：orientation = 本次物理输入下标 → 屏幕下标映射
否则 orientation = null；confidence/marker_code = null
```

不得先按 double 输出顺序转换后丢掉 P 标签，再猜物理对应。不得把已舍入点写回 measurement/evidence。float 发布检查只检查表示合法性，不新增尺度/角度阈值，不引入像素 epsilon；沿用严格凸、范围 `[0,width)×[0,height)`。

任一 retained measurement 发布失败时，整次 decode 返回 NOT_DETECTED，清本次待发布 detections、保留原始 measurements 和明确失败原因；不能删除失败竞争后把其他项声明唯一。先全部建立临时发布集合，再一次提交，避免半成功结果。

### 2.3 Step 0 验证（必须通过才能继续）

| ID | 输入 | 预期/断言 |
|---|---|---|
| P01 | 中心150/150，边100；44.999999°、45.000001°，再加90/180/270°；known/unknown各测 | helper 输出经 float 域共享排序不变；合法输出被 TemporalStabilizer 接受；unknown 始终空 |
| P02 | 已知 P0–P3 的旋转角与0–345°每15° | 按 orientation 重建物理 float 坐标，逐点等于 measurement 直接转换；不移动点；bbox准确 |
| P03 | 舍入前不同、舍入后重合；NaN/Inf；float 舍入到 width/height 边界 | 发布失败，明确原因，不伪造点或裁坐标 |
| P04 | 45°精确平局 | 使用原排序字典序，确定输出；物理映射仍正确；不得用 tie 单独取消已证实的物理方向 |
| P05 | 既有完整测试 | Release **19/19**；相关主动测试 Debug 通过；旧用例数量/断言不删除 |
| P06 | 同一视频0–1675，补丁前/后逐帧 | 1676记录；比较 raw status/count、点集合、物理映射、证据、拒绝原因和截断；允许的差异只有批准的规范置换及其映射、发布失败诊断 |

当前视频历史 raw864/empty812，预计本补丁保持这些数量，但数量相同不是验收充分条件。允许的置换需逐帧列清；当前视频若出现状态/检测数量变化或不在批准类别内的差异，**停止 Step 0，报告 G-UNEXPECTED**，不得自行接受“改善”或调阈值。

稳定结果不得被删、unknown不得回填、偏离不得超过批准2px，按 Block4 核查规则再次检查。旧归档数据不覆盖；产出 `step0/route_b_comparison.json`、失败索引、逐帧结果及 `patched_baseline_manifest.yaml`。后续所有非回归对照以此补丁后基线为准，不再把已批准点序变化报为 Block5 回归。

## 3. 架构：六模块内增加观察通道

### 3.1 职责

| 归属 | 新职责 |
|---|---|
| core | Stage/TimingStatus、通用 FrameRecord、时钟与枚举事件。Geometry/Decode/Temporal 的具体详情不下沉 core |
| pipeline | 实例诊断上下文、阶段详情、内部访问器、recorder、run report；拥有装配，不能持有全局状态 |
| corners | Route B 发布 helper；后续取证算法锁定 |
| config | 严格加载/校验/导出观测和运行配置 |
| preprocess/geometry | 算法不改；计时在调用处包围，复用已产生结果 |
| app/ | 离线输入、模式、绘制、等待、证据导出；不进入 Detector 核心 |
| tools/ | 阶段 audit 和验证工具；使用同一 schema/serializer，不各写一套日志 |

不增加通用 utils；app 不是第七个 lib 算法模块。新文件目录不得写成原冻结旧平铺路径。

### 3.2 FrameRecord 内部通道：实例持有，显式访问

不往 FrameResult 增加新字段，不改 diagnostics 类型。不用全局/tls、不用回调劫持 public process、不增加公共 observer/setter 方法。

在 `mark/detector.hpp` namespace 内前向声明 `class DetectorDiagnosticsAccess;`，在 Detector **private** 增加 friend。这不增加数据成员、不改对象布局、不改公共四调用签名。类型定义只放内部头，不将 lib include 变 PUBLIC。

```cpp
class DetectorDiagnosticsAccess {
public:
    static void prepare(Detector&, const DiagnosticsRequest& request);
    static FrameRecord take(Detector&); // 无本次记录或二次take：logic_error
};
```

Impl 拥有 `FrameDiagnosticsContext` 和一份完成记录。prepare 设置本次输入的 run_id、采样请求及执行 scope；禁止处理期间调用、禁止换另一个实例取记录。process 创建独立记录，即使非法/NOT_READY也有；prepare 不是就绪条件，没有 prepare 时采用禁用采样的安全内部默认，旧公共用户仍能正常调用。take 只移动诊断载荷，不改变算法三历史。

同一 Detector 不承诺并发 process/prepare/take；两个实例互不影响。一次处理完成后、下一次 process 前必须取走 app/audit 所需记录；覆盖未取记录标记为诊断消费者错误，执行器失败，不能静默丢帧。普通不使用内部访问器的公共调用不因此失败。

内部 `runDecodePipeline()` 保留旧签名作 wrapper，增加带 `FrameDiagnosticsContext*` 的内部重载；`decodeStage()` 同样保留旧签名，加带 context 的重载。只在编排处报告类型化结果/事件；底层算法函数签名与判定不改。现有工具调用旧签名仍可编译，但迁移后的三个 audit 必须用观测重载。

详细载荷属于内部 `StageDetails`：复用 `WhiteComponent`、`ShapeObservation`、`GeometryBatch`、`CornerMeasurement`、`TemporalDiagnostics`，不用另一份 CornerEvidence。仅详细选中帧复制；cv::Mat 仅持有真实结果的只读引用计数副本，不能改写或存预测点。源图留给 app 按需外存。

core的FrameRecord只前向声明StageDetails，并持有`std::shared_ptr<const StageDetails>`可选详情；StageDetails完整定义位于pipeline/diagnostics_context.hpp。这样core不include pipeline/corners头，生产摘要不分配StageDetails。FrameRecord通用字段可include既有公开结果类型，不新增公共结果字段。所有计时槽显式初始化，非scope项NOT_EXECUTED、scope中未进入项SKIPPED、计时关闭的执行项DISABLED，不能依赖未初始化enum/optional。

### 3.3 process_total 的完整边界

process 入口第一项创建计时作用域，整个函数的输入检查、预算检查、实际阶段、结果组装与诊断构造都在其中。RAII 离开函数时记内部 total；其后无字符串构造/序列化/导出。正常/提前return/异常都有记录；异常另记 run失败，不伪装 INVALID_INPUT。

App 在调用 `Detector::process()` 前后用同一 steady_clock 包围一次完整公共调用，得到完整调用时间，**作为 full scope 最终 process_total**，覆盖内部近似 total，不重复累计。它包括返回及内部收尾成本，无需声称能在函数内部量到自身最后一次析构。审计未调用公共 process 时 process_total=NOT_EXECUTED，绝不能给共享装配假冠名。

内部 total用于未绑定 app 的内部诊断，标 `process_total_boundary=internal_scope`；App 报告为 `public_call`。正式14ms报告必须为 public_call。重复覆盖是替换一个样本，不是两项相加。

## 4. 文件清单与修改边界

以下表格路径均从仓库根开始，斜线规范使用 `/`。

### 4.1 新增

| 完整路径 | 责任 |
|---|---|
| `src/tushenghao/lib/corners/detection_publication.hpp/.cpp` | Step0 float 规范发布 |
| `src/tushenghao/lib/core/diagnostics_types.hpp` | 通用记录、计时状态、原因枚举/事件元数据 |
| `src/tushenghao/lib/core/timing.hpp/.cpp` | FrameTiming、ScopedStageTimer、受控计时接口 |
| `src/tushenghao/lib/pipeline/diagnostics_context.hpp/.cpp` | 内部请求、阶段详情、帧记录装配、D02重置事件 |
| `src/tushenghao/lib/pipeline/detector_diagnostics_access.hpp` | 内部 prepare/take 声明；定义在 detector.cpp，不泄露 Impl |
| `src/tushenghao/lib/pipeline/diagnostics_recorder.hpp/.cpp` | 生命周期、统一JSONL、汇总采样、错误处理 |
| `src/tushenghao/lib/pipeline/run_report.hpp/.cpp` | 统计、manifest/summary、有效配置快照规则 |
| `src/tushenghao/app/debug_renderer.hpp/.cpp` | 纯渲染函数及独立副本 |
| `src/tushenghao/app/offline_runner.hpp/.cpp` | 公共流程、capture/visualize/wait/export、模式执行 |
| `src/tushenghao/lib/config/observability_config.hpp` | DiagnosticsConfig/RenderConfig/OfflineRunConfig 值类型；只依赖std/basic types |
| `src/tushenghao/tools/audit/geometry_audit.cpp` | 新 geometry scope 阶段工具 |
| `src/tushenghao/tools/validation/observability_verify.cpp` | 三audit schema、baseline/debug、Step0及归档一致性核验；不参与检测 |
| `src/tushenghao/tools/common/frame_record_reader.hpp/.cpp` | 仅测试/验证工具使用的受控JSON记录读取，标准库实现；不链接进Detector |
| `src/tushenghao/tests/timing_test.cpp` | 计时状态、统计、提前返回/异常 |
| `src/tushenghao/tests/diagnostics_record_test.cpp` | JSONL/报告/文件失败/事件 |
| `src/tushenghao/tests/debug_renderer_test.cpp` | 输入不变、图层、empty/history/unknown |
| `src/tushenghao/tests/observability_integration_test.cpp` | 内部通道、阶段、配置、实例/重置、模式结果一致 |
| `src/tushenghao/test_support/observability_fixture.hpp` | 小型主动断言fixture，不填生产预算 |
| `src/tushenghao/docs/diagnostics_schema.md` | schema、单位、兼容与时钟口径 |
| `src/tushenghao/docs/block5_acceptance.md` | 完成内容/验证方法/踩坑记录/已知限制，实际耗时/剩余项 |
| `src/tushenghao/docs/evidence/block5/INDEX.md` | 全部本轮证据导航及基线关系 |
| `src/tushenghao/docs/evidence/block5/` 下固定子目录 | step0、tests、runs、comparison、environment、archive，证据落位 |

表中 `.hpp/.cpp` 表示同名两个文件，不能创建带斜线的文件名。

### 4.2 修改（精确允许范围）

| 完整路径 | 允许 |
|---|---|
| `src/tushenghao/lib/pipeline/decode_stage.hpp/.cpp` | Step0调用；保留旧内部入口并加context重载；各阶段计时、原因与详细记录条件构造 |
| `src/tushenghao/lib/pipeline/detector.cpp` | 实例记录通道、全调用内部计时、D02 reset事件、读取时序诊断；算法顺序和门控不改 |
| `src/tushenghao/include/mark/detector.hpp` | 私有friend及内部类前向声明、说明；公共签名不改 |
| `src/tushenghao/include/mark/detector_types.hpp` | 只整理过期 diagnostics 注释；任何类型布局不改 |
| `src/tushenghao/lib/pipeline/temporal_stabilizer.hpp/.cpp` | D02内部reset原因快照/事件访问；不改关联、对应、alpha和稳定判定 |
| `src/tushenghao/lib/pipeline/stabilize_stage.hpp/.cpp` | 保留原签名，增加context重载；非法分支先复制本次诊断，再reset，不丢事件 |
| `src/tushenghao/lib/config/app_config.hpp` | 增应用级 Diagnostics/Render/OfflineRun配置 |
| `src/tushenghao/lib/config/config.cpp` | 新字段名单、加载、校验、导出；解禁已实现Block5开关；旧合法字段不删除 |
| `src/tushenghao/config/detector.yaml` | 仅 debug/output观测/运行选项；算法/时序数值原样保留 |
| `src/tushenghao/tools/audit/decode_audit.cpp` | 共用serializer/recorder；保留旧位置参数及阶段行为说明 |
| `src/tushenghao/tools/audit/temporal_audit.cpp` | 共用schema；保留video/experiment-grid与原网格，不改预算实验规则 |
| `src/tushenghao/tools/common/temporal_record.hpp` | 移除独立JSON格式；可留转调公共serializer的兼容薄层，无第二实现 |
| `src/tushenghao/app/main.cpp` | 保留--check-config，新增离线入口 |
| `src/tushenghao/CMakeLists.txt` | 注册源、4个新普通CTest、geometry_audit/verify；测试渲染独立连接app renderer；digest覆盖新源、无Git |
| README/docs/INDEX/tools/INDEX/evidence/INDEX | 中文导航、命令、已知限制、归档说明 |
| 既有配置/Block3/Block4相关测试 | 追加批准行为/配置用例，不删旧覆盖、不过度重写 |

禁止修改 geometry 的匹配/验证/补全算法、preprocess算法、CornerResolver/边拟合/证据验证/semantic规则、生产模型、v0.1、冻结ref原文。新增观察只在已有返回结果处提取，不往算法内加新判据。

## 5. FrameRecord、事件与统一序列化

### 5.1 类型与单位

```cpp
enum class Stage { Capture, Preprocess, Detect, Decode, Stabilize,
                   ProcessTotal, Visualize, Wait, Export };
enum class TimingStatus { MEASURED, DISABLED, SKIPPED,
                          NOT_IMPLEMENTED, NOT_EXECUTED };
enum class ExecutionScope { Full, Geometry, Decode, Temporal };
enum class DiagnosticsLevel { Summary, Frame, Evidence };
struct StageTiming { Stage stage; TimingStatus status;
                    std::optional<std::chrono::nanoseconds> elapsed; };
```

FrameRecord **schema=1**（与旧日志不同，无推定旧schema）；配置 `schema_version=1` 保持兼容，是另一种版本。

| 字段组 | 内容与语义 |
|---|---|
| 身份 | record_schema_version、run_id、frame_id、source_timestamp_us、timestamp_source、timestamp_recipe、execution_scope |
| 尺寸/状态 | original_size/work_size、result_status；geometry scope result_status=null，用 geometry_scope_result=READY/NOT_READY/INVALID_INPUT，不能把有假设冒称DETECTED |
| 计时 | 九个stage固定集合；elapsed输出us，小数保留，不先截为整数；只有MEASURED含值 |
| 事件 | stage、ReasonCode、detail原文、可选hypothesis/component ID、来源帧；外部reset可 source_frame_id=null，附着下一输入时不得冒称该输入触发 |
| 计数 | components、observations、generated/validated/completed hypotheses、measurements、detections、tracks、truncated；未执行计数null，不填0 |
| 当前输出 | frame/evidence级：raw detections、stable tracks、display_state；summary只计数，不保存几何副本 |
| 阶段详情 | 可选 GeometryDetails/DecodeDetails/TemporalDetails；复用真实类型，measurement包含原CornerEvidence |
| 条件实时项 | slot_overwrite_count、consumed_frame_count、enqueue_timestamp_ns、enqueue_to_result_us、clock_domain；offline全部null，附not_applicable |
| 完整性 | run_failed、exception/IO事件、实际消费/记录/导出数量，采样策略 |

uint64帧号/计数、int64时间戳若写JSON，统一十进制字符串，避免下游FileStorage double丢失大整数；schema记录这一约定，验证工具用from_chars校验。尺寸及枚举为JSON整数/字符串；点/时间差/权重为有限数字。

原因枚举至少覆盖 InputFormat、InvalidStamp、InvalidSequence、BudgetMissing、ClearlyIncomplete、AssignmentInsufficient、CornerRejected、ScreenRejected、ValidationRejected、UnresolvedCompetition、SemanticRejected、PublishFloatRejected、HistoryExpired、AssociationFailed/Ambiguous、CorrespondenceAmbiguous、ZeroDt、SmoothingRejected、ResetExternal/InputChanged/InvalidSequence、NonFiniteField、IoFailure、OtherRecordedReason。

阶段包装对已知原因产生枚举事件；不可安全细分的旧文本用 OtherRecordedReason 携带原文。**禁止靠匹配字符串猜未测量的物理原因**，如把角拟合失败写成遮挡。历史帧被拒绝的118不变成置信度低放行。

### 5.2 D02 重置事件

`reset(ResetReason)` 保存无分配的内部原因枚举/计数快照，清三历史；重复reset记录各原因计数，保留最后原因，计数溢出饱和并标记，不在noexcept函数抛异常。详细事件vector/string在后续process/工具处构造。

内部 reset 不能覆盖当前帧已经记录的失败事件。finalize 在清理前复制 TemporalDiagnostics 到context，之后清历史；非法返回保留失败原因。外部reset由Detector和Temporal两层发生的同一调用只记一条语义事件，不双计；具体以Detector为公共事件拥有者，直用Temporal的audit则由Temporal拥有。

DisplayHistory不新增第二套事件标准。尺寸变化、显式换源、非法恢复均记录真实调用来源，依旧不传生产假语义文本。

### 5.3 兼容与诊断开销

`FrameResult.diagnostics` 字段保留；准入/拒绝/回退的既有字符串原因继续保留。**逐父假设 affine/assignments 的长trace**属于详细诊断，只在选中 frame/evidence 请求时构建；summary不构建。此变化是观测级别控制，不是算法变更，必须在schema/README列明。

非回归逐帧比较有效结果及规范原因事件；不要求 timing、run_id、hash、重置新增事件、详细trace数量逐字相同。不得笼统排除全部diagnostics来掩盖原因丢失：准入原因、截断与原始CornerEvidence另行严格比较。

process_total包含内部诊断事件、计数及详细复制；不得扣“估计日志成本”。serializer/文件写/图像编码发生在app/audit外部 export。输出额外诊断构造开销只能以实际包围的区间报告；不通过“debug_total−baseline_total”声称精确拆出日志成本。

### 5.4 serializer/recorder

```cpp
class FrameTiming {
public:
    void markStatus(Stage, TimingStatus);
    void addMeasured(Stage, std::chrono::nanoseconds);
    const TimingSnapshot& snapshot() const noexcept;
};
class ScopedStageTimer {
public:
    ScopedStageTimer(FrameTiming&, Stage);
    ~ScopedStageTimer() noexcept;
};
class DiagnosticsRecorder {
public:
    explicit DiagnosticsRecorder(DiagnosticsConfig);
    void beginRun(const RunMetadata&);
    void submit(const FrameRecord&);
    RunSummary finishRun();
};
TimingStatistics summarizeDurations(
    const std::vector<std::chrono::nanoseconds>&);
```

RAII构造前先校验stage状态；析构只写预留槽，不分配/打印/IO/抛异常。内部嵌套允许，单stage每帧一次，重复开始/覆盖须被主动检查拒绝。App覆盖ProcessTotal使用独立明确replace操作，不伪装addMeasured第二次累计。计时关闭初始化DISABLED；只有真的进入阶段才MEASURED。

JSONL专用serializer唯一实现；quote/control字符转义、UTF-8原样、optional=null、无NaN/Inf/locale逗号。非有限字段写null并追加NonFiniteField，不静默当0。无新JSON/YAML依赖；不要假定FileStorage能读任意含null的JSON。`frame_record_reader`用标准库实现受控记录的对象/数组/string/有限number/bool/null读取，拒重复键、尾随内容、非法转义和截断，支持Unicode转义含代理对；schema校验与语法读取分开。它只服务测试/verify，不进入算法库；不演变为万能JSON依赖。旧fixture已有依赖不扩散。

Recorder summary内存保存紧凑各stage时长/计数，不保存全视频 evidence。frame/evidence只有选中输入输出frames.jsonl，**所有实际处理帧仍进入统计**。内存不足显式失败，不抽样计时、删慢帧或漏首帧。

统计：N、mean、median（偶数中间两项平均）、nearest-rank p95/p99（1-based ceil(pN)）、max；N=0全统计null；N=1各分位即该值。只用MEASURED，其他状态分别计数。各阶段p95不能相加当total p95。

run目录：manifest.yaml、effective_config.yaml、summary.yaml、按需frames.jsonl/evidence。已有目录拒绝；begin/submit/finish顺序及二次finish非法明确失败；失败写标记和已完成样本数，不伪装完整报告。stdout仅运行摘要/错误，不夹杂第二套逐帧JSON。

manifest含代码提交标签（外部b9cccd4，不能冒称修改后提交）、实际源hash、输入指纹、配置/模型版本、CPU/OS/compiler/OpenCV/build type/threads、模式、尺寸、fps及时间戳公式、选中策略和样本数。环境获取失败填UNKNOWN+说明，不编造CPU。审计沿用已有OpenSSL SHA256；Detector库及新核心不链接Crypto。App使用标准库实现明确标识的FNV1a64+文件大小指纹（非密码学hash），归档验证工具补SHA256并关联，不把FNV冒称SHA256。保留现有工具依赖，不新增第三方库。

## 6. 阶段插点、模式与配置

### 6.1 实际调用边界

| stage | 插点 | 提前返回/范围规则 |
|---|---|---|
| capture | runner/audit每次read开始→返回图像 | EOF的最后失败read单列run事件/开销，不算一张算法帧；读到零帧失败 |
| preprocess | runDecodePipeline内preprocess调用 | 无合法输入/缺预算则SKIPPED |
| detect | extractWhiteComponents→observeShapes→generate→validate→completeSegmentedAssignments | 按原顺序一次；中间详情复制与事件构造归在实际执行阶段，不隐藏 |
| decode | decodeStage入口→返回，含RouteB发布 | 无geometry范围工具则NOT_EXECUTED |
| stabilize | finalizeDecodedFrame调用，含装配/显示更新 | geometry/decode audit未调用则NOT_EXECUTED |
| process_total | full App包围一次公共process | 阶段audit未调用公共入口=NOT_EXECUTED |
| visualize | render及可选imshow提交 | 没画图=NOT_EXECUTED，不当0 |
| wait | waitKey/离线限速等待 | 无等待=NOT_EXECUTED |
| export | 每帧JSONL/证据图写调用 | 与绘制/算法分离；summary结束文件写单列run_export，不塞最后帧 |

阶段和差额只在相关阶段均MEASURED时报告框架overhead；缺任一则差额null。内外不同计时区间不得混算。首次 lazyTemporal构造/model已在constructor加载等，按真实边界计，constructor启动成本单列，不摊帧。

导出计时自引用处理：对本帧图像+记录的序列化/写调用实际计时，`frames.jsonl`该行先保持Export为待完成的NOT_EXECUTED及说明；写后记录测得Export进内存统计，frame/evidence另统一写 `export_timings.jsonl`，以run_id/frame_id关联。侧表本身及finishRun写入作为run_export统计，不递归再给自身造记录。schema明确侧表是同一记录的计时补全，**不是第二套audit标准**。baseline没有逐帧文件，Export样本仅实际发生时才有。

### 6.2 离线 App

保留 `marker_app --check-config`，支持可选`--config`；新调用：

```bash
./marker_app --video <输入视频> --config <detector.yaml> --run-dir <新目录> --mode baseline
./marker_app --video <输入视频> --config <detector.yaml> --run-dir <新目录> --mode debug
```

CLI仅覆盖路径/模式，不临时覆盖算法参数。未知/重复/缺参数退出非零；默认无参打印帮助，不能退出0而什么都没运行。相对CLI路径按调用CWD解析；已有loadConfig的模型相对路径规则保持，不悄改模型定位。manifest记录解析后的路径、CWD与指纹；README给从任意CWD的明确可用命令。

每一实际读帧按id从0递增，算法timestamp=llround(id×1e6/fps)，TimestampSource保持当前合法Unknown；FrameRecord注明video_fps来源。fps非有限/≤0、换算溢出、read异常显式失败。实际读帧为分母，CAP_PROP_FRAME_COUNT仅元数据比较，不能默认可靠EOF判据。归档指定视频必须1676帧，不足算失败。

baseline：完整公共算法/稳定默认开启，不显示、不绘制、不存详细证据，无waitKey；summary+timing必须开，冲突配置拒绝，不静默改变用户配置。保存首帧单列且纳入全统计。吞吐注明read/process/export等真实墙钟区间，不叫曝光延迟。

debug：同算法配置和输入帧集合；范围/interval只选择**详细记录和证据导出**，不是选检测帧；渲染层开关可以独立。证据帧必须先被细节请求选中；候选来自当前工作图，绘制时通过PreparedFrame原有像素中心映射转原图，不用简单宽度比例偷换坐标。

可选imshow仅debug；GUI不可用显式报错/非零，不悄忽略请求。waitKey在算法外，用户提前退出记incomplete，不称全视频通过。视频导出本期只保留配置，启用时报NOT_IMPLEMENTED错误，不额外实现编码器选型。

### 6.3 YAML（全部加载/校验/导出/测试同步）

沿用原root/output/debug层级；新观测值存AppConfig，不把新视频输入行为塞DetectorConfig。旧 debug.timing_enabled/draw_candidates、output.show_window/show_held_state继续解析到旧成员，转换到App配置；不能出现两个不同生效值。旧schema1缺新字段使用以下观测默认，算法门限无新增默认。

| 路径 | 值/约束 |
|---|---|
| debug.timing_enabled | 旧bool保留；交付生产设1（观测默认），关闭则计时DISABLED；baseline拒关闭 |
| debug.level | summary/frame/evidence，默认summary |
| debug.detail_first/detail_last | 非负帧id；last可省略表示无限；闭区间，first默认0，last≥first |
| debug.detail_interval | 正整数，默认1；命中条件(id-first)%interval==0 |
| debug.draw_raw/draw_stable | 默认1，实际只在debug绘制 |
| debug.draw_candidates | 旧bool默认0；工作观测按映射绘制 |
| debug.draw_corner_evidence/debug.draw_timing | 默认0；证据图层要求选中详情，否则该图层空且说明未采样 |
| output.run_mode | baseline/debug，默认baseline；CLI可覆盖此唯一运行模式 |
| output.run_directory | 默认空，必须由YAML或CLI给新目录；不依赖用户个人路径 |
| output.show_window/output.show_held_state | 旧bool默认0；held只绘显示语义、不启用历史算法 |
| output.export_evidence | 默认0；仅debug+evidence，原图及overlay分别保存 |
| output.export_video | 默认0；1显式未实现错误 |
| output.playback_fps | 默认0不等待；debug下0或有限正数，baseline必须0 |

所有bool仅现有0/1规则；整数溢出/错误类型/未知重复字段沿旧严格规则拒。配置解析通过不表示GUI可用；执行时硬件/文件失败如实报。run_directory/帧范围等应用值不影响Detector.config算法门限。writeEffectiveConfig包含最终CLI覆盖后的生效App选项，不只导出原YAML。

实时字段仅预留，本期无采集线程/帧槽。offline null/N/A；已有适配元数据同steady_clock域才算enqueue延迟，无元数据不制造计数。不得将视频fps时间减steady_clock时间。

## 7. 三 audit 迁移与绘制

### 7.1 audit

| 工具 | 实际执行 | 新统一记录 |
|---|---|---|
| geometry_audit | 同preprocess+完整detect链到M/S补全；不decode/stabilize/publicprocess | scope=geometry；假设/截断/原因真实；无Detection含义 |
| decode_audit | 同runDecodePipeline，不stabilize/publicprocess | scope=decode；保留原始CornerEvidence；public_status=NOT_EVALUATED |
| temporal_audit video | 同一次runDecodePipeline+finalize | scope=temporal；raw/stable/三状态，process_total NOT_EXECUTED |
| temporal_audit experiment-grid | 原有限合成语义序列直接喂稳定层 | scope=temporal；preprocess/detect/decode NOT_EXECUTED；truth仅工具详情，不进入Detector |
| marker_app | 一次真正Detector.process | scope=full；可用于14ms基线 |

geometry_audit CLI明确采用 `--video --config --run-dir`；支持debug.level/range/interval，同一完整处理顺序；不用新算法断言证明目标正确，只检查阶段有效性、source/assignment引用及记录一致。

decode_audit旧位置参数保留；增加 `--run-dir`。旧帧列表明确仍是阶段工具执行子集，用execution_subset披露，不可称全视频基线；新 `all` 模式逐帧执行。不保留旧独立stdout JSON，README列迁移说明。

temporal_audit保留--video/--experiment-grid/--noise-csv/--config/--overwrite；新增--run-dir。旧--output兼容为所给文件旁一个新的 `<文件名>.run/`目录中的frames.jsonl，并写兼容提示，原路径只写指向新run的说明文件；不得覆盖旧证据，--overwrite也不能覆盖docs/evidence旧run。网格仍480段×32，原sourcehash/noise provenance进入统一manifest和experimental详情。不重新实施G-B审批。

所有audit迁移后使用同一FrameRecord、serializer、summary；保留原阶段assert/核查，不把输入特有字段混成全流程。旧日志和Block3/4复核脚本保留原意；新observability_verify读取新版，不追改旧档案。

### 7.2 绘制

```cpp
cv::Mat renderDebugFrame(const cv::Mat& original,
    const FrameResult& result, const FrameRecord& record,
    const RenderConfig& config);
```

每次original.clone；原图不改，不复用前一overlay。原始RAW青色实线、稳定STABLE绿色虚线/标记，图例写来源；颜色是展示常量，不是算法参数。当前empty/INVALID_INPUT/NOT_READY不绘任何有效框，即使传入不一致载荷也先按状态防御。

屏幕LT/RT/RB/LB与P0–P3分别标；unknown只有屏幕标签和orientation=UNKNOWN，不借历史加P标签。display_state非空且show_held_state时绘文字；held=true必须HISTORY+source frame+age；非held写CURRENT DISPLAY。不绘历史四点、不得借绘制给Detection填属性。

候选层允许empty帧显示当前候选，但标题CANDIDATE，不能画成有效检测。所有证据图层用当前帧id匹配，错帧禁止绘并记录错误。绘制函数不imshow/waitKey/写文件；runner承担提交和IO。

## 8. Step 1–8 执行顺序与产出

先完成Step0，才进行以下步骤。时间只记录实际，不授权删减范围。

| 步骤 | 具体任务 | 产出/检查点 |
|---|---|---|
| 1 Schema/通道 | 定义类型、friend access、context、原因和D02事件；保留公共布局 | 编译、旧公共消费测试、非法/NOT_READY记录、实例隔离及reset事件主动断言 |
| 2 阶段计时 | 在实际编排插点，App public-call total，异常/earlyreturn状态 | 九stage固定槽；三scope不冒称publicprocess；wait隔离和重复计时负例 |
| 3 报告 | summary/manifest/effectiveconfig、内存紧凑统计、首帧和全帧 | 人工已知样本统计正确、无样本null、无漏首帧、目录/文件失败非零 |
| 4 统一日志/audit | controlled JSONL、export补全侧表、迁移两audit、新geometryaudit | 三份schema字段一致，旧断言保留，无第二serializer，显式scope/子集 |
| 5 绘制 | renderer、独立历史文本、工作图候选映射 | 输入hash不变、empty清层、unknown不回填、错帧证据拒绘 |
| 6 App/配置 | 离线runner、两模式、YAML全链、条件实时字段 | baseline/debug算法相同；路径/GUI/输出失败明确；offline不需实时项 |
| 7 视频实测 | 补丁基线vs最终baseline/debug；0–1675全帧逐一比 | 实际性能报告、原因/证据/稳定一致、诊断采样不跳算法帧 |
| 8 交付/归档 | 全回归、中文README/schema/acceptance/INDEX、hash/清理 | 完整证据归档核查通过后删除本任务build；不能只写“23/23全绿” |

每步在acceptance记“开始/结束、实际主动耗时、执行命令/退出码、产物相对路径、失败/修复、剩余项”。测试失败先保存失败日志再修，不能覆盖为只剩PASS。

## 9. 测试计划（主动断言，保留所有旧测试）

新增4个普通CTest程序，预期总数 **23**；Step0阶段仍19。不能依赖assert在Release运行；用现有check/异常/非零退出机制，不引入GTest。

### 9.1 timing_test：M01–M08

| ID | 输入 | 断言 |
|---|---|---|
| M01 | 已知duration 1..20ms | N20、mean/median10.5、p95=19、p99/max=20；不相加阶段分位 |
| M02 | 空；单个7ns | 空统计null；单个所有分位7ns；序列化0.007us非截0 |
| M03 | MEASURED/DISABLED/SKIPPED/NOT_IMPLEMENTED/NOT_EXECUTED混合 | 只测量值入统计，其余各计数、elapsed=null |
| M04 | 正常、return、throw的RAII作用域 | 到退出都有记录且非负；throw仍标run失败、不假成功 |
| M05 | 同stage重复start/add；禁用状态 | 重复测量主动拒绝；禁用无clock读/测量值 |
| M06 | controlled wait在process外；process内已知工作 | wait不计process；不使用精确睡眠时长作脆弱断言，比较timer的区间/状态及逻辑不重叠 |
| M07 | 明确外部publiccall替换internaltotal | 只有一个ProcessTotal样本，boundary=public_call，不两次累计 |
| M08 | 含一个慢样本/首帧 | slow与first均保留分母，first独立栏仍计总N |

### 9.2 diagnostics_record_test：R01–R09

| ID | 输入 | 断言 |
|---|---|---|
| R01 | 中文、引号、反斜线、换行、控制字符 | JSON逐行可解析并还原，UTF8保留 |
| R02 | absent、0、NaN、Inf、大uint64/int64 | absent=null；合法0保留；非有限null+事件；大整数十进制字符串无精度损失 |
| R03 | lifecycle非法顺序/已有目录/打开写入失败 | 非零/异常，旧文件hash不变，无“成功summary” |
| R04 | summary/frame/evidence相同100次submit，细节选10次 | 汇总N100；详细仅10；summary无frames/证据；计数未被采样稀释 |
| R05 | 九stage与四scope | 未运行stage NOT_EXECUTED，缺预算后未进stage SKIPPED；无未执行0ms |
| R06 | export侧表，run final export | frameid关联唯一，side计时补全入summary；侧表自身不递归计时 |
| R07 | 外部三种reset/重复reset/非法finalize后恢复 | 真实原因保留、无双计/旧历史恢复，源帧未知null |
| R08 | 同一CornerMeasurement通过详情序列化 | 既有证据字段/支持弧/物理ID不改，不另造证据 |
| R09 | finish后核对manifest/配置重载 | source/model/input指纹和模式可追溯，CLI有效覆盖导出；无CPU猜值 |

### 9.3 debug_renderer_test：V01–V07

| ID | 输入 | 断言 |
|---|---|---|
| V01 | 黑/有目标原图，RAW/STABLE层 | 原图逐字节不变，输出独立buffer，图例与差异点存在 |
| V02 | 有效后一帧empty；另INVALID_INPUT/NOT_READY故意附旧载荷 | 当前有效图层空，不能留旧四点/框 |
| V03 | 历史A source0 age5 held | 仅文字HISTORY/来源年龄，检测/track仍空 |
| V04 | 当前unknown+旧known上下文 | 无P标签/旧方向，屏幕标签可在当前raw上绘 |
| V05 | 480/960工作坐标候选 | 使用PreparedFrame像素中心映射；选具体非整数点比golden，不只检查“看着对” |
| V06 | 错帧evidence、未选细节 | 错帧不画、事件明确；未采样不造证据 |
| V07 | 各层关闭 | 无对应图层；renderer没有waitKey/文件/VideoCapture调用 |

### 9.4 observability_integration_test：I01–I10

| ID | 输入 | 断言 |
|---|---|---|
| I01 | 公共消费只用原4调用；内部prepare/process/take | 原公共源码可编译；布局字段未加，take二次/错时序失败 |
| I02 | 非法图像、重复id、缺预算、黑帧、完整fixture | 结果遵原状态；记录每次存在，阶段状态准确；非法后低id恢复raw首帧 |
| I03 | 两实例交错；只reset A | B结果/原因不受A影响；无全局observer |
| I04 | 相同多帧fixture，timing关/开及三级诊断 | raw/tracks/orientation/confidence/marker_code/display逐字段相同；准入原因相同 |
| I05 | baseline/debug配置冲突及新字段边界 | baseline不开GUI/等待/证据；错误配置拒绝，新配置往返一致 |
| I06 | detailed first=2,last=8,interval=3，处理0..9 | 处理10帧，详细仅2/5/8；tracks历史不中断 |
| I07 | threeaudit scope及真实geometrybatch | 同schema/serializer；几何不报告DETECTED，stage区分明确 |
| I08 | 离线无实时元数据；另clockdomain不同fixture | null/N/A，不混时钟、不伪造延迟/覆盖计数 |
| I09 | D01 helper产物进入公共阶段与Temporal | 接受规范raw，物理方向映射正确；无精度冲突回归 |
| I10 | 输出错误/EOF零帧/提前退出/无GUI请求 | 错误非零/incomplete明确，不能当全视频PASS |

### 9.5 全视频与阶段工具必跑

1. 指定输入 hash，0–1675全1676帧；Step0补丁基线、最终App baseline、最终App debug各独立run。每个run重建Detector，从id0开始，不复用历史。
2. 默认debug详细范围全帧、interval1，绘制可离屏，不强制GUI；逐帧导出新schema。最终baseline只summary，验证工具另使用同参数和公共process的验证运行导出全帧结果；标scope full、`run_purpose=verification`、mode debug，不新增第三种生产模式，不冒称这些日志来自无详细baseline。baseline有效结果用独立紧凑指纹链和状态计数作连接，逐帧正确性由验证运行核查。
3. 对照补丁基线：raw点/bbox/orientation/质量/类别、空置信度/编码、稳定点/当前索引/display、准入原因与截断逐帧相同；计时、hash/runmeta/详细trace及新增观测事件排除并明列。无有效点/属性差异必须为0。
4. 原始证据在选中全帧debug/审计中与patched基线逐字段相同；不重新fit或重写原始误差。稳态对照优先exact float；跨环境差异不能临时放epsilon，报告G-UNEXPECTED。
5. baseline统计N1676、首帧仍在全量、没有隐藏跳帧；DETECTED/NOT_DETECTED计数应与patched基线一致，NOT_READY/INVALID_INPUT正常视频为0；每检测最多1track，empty无track、display默认null，r/偏离2px unchanged。
6. geometry/decode/temporal三个audit至少一次all，核对实际scope/帧数/跳过阶段/统一schema；decode原测量对应App raw；temporal共享装配对应App stable。geometry不以候选数当正确检测率。
7. 报告process_total mean/median/p95/p99/max，阶段分布/框架开销、capture/visualize/wait/export额外成本、吞吐和覆盖；结果未达14ms如实标未达，不降低验收为“速度差不多”。

所有旧Block4主动30组、旧Block3回归继续保留；不只看CTest程序数。若已有H720输入可用，复用跑H720并归档；不可用明确记缺失，不生成巨大新网格替代，不将H720称本轮通过。新Block5强制项不凭空增加重新标注视频或置信度数据工作。

**紧凑结果指纹的确定规则：** FNV1a64，初值14695981039346656037，逐字节乘1099511628211并按uint64溢出。按帧依次输入frame_id/timestamp/status、raw数量与每项category/quality/corners/bbox/orientation/marker_code/confidence、track数量/索引/同字段、display可选值。整数固定little-endian，float用memcpy取IEEE位模式后little-endian；optional先写0/1，string先长度后UTF8字节，vector先数量；不输入diagnostics/timing/runmeta。不建JSON/几何副本、不保存每帧对象；计算在process外，列入runner bookkeeping开销。summary记录算法名/版本、最终hex值、N；baseline/debug及patched基线相同即快速非回归连接，**指纹不是逐帧比对的替代**。verify仍必须在有逐帧记录的验证run中定位差异，不能仅凭hash宣布全部正确。

## 10. 验收、证据归档和清理铁律

### 10.1 必须达到才可写“Block5完成”

- Step0 P01–P06及19/19通过，允许差异被逐帧分类；patched基线归档。
- 新4个CTest及全部既有测试通过（预期23/23），主动M/R/V/I组全部通过；缺失/失败不改表变绿。
- D02真实reset事件保留；公共有效输出与三历史行为不变。
- 内部实例通道正确，公共布局/4签名不变；diagnostics兼容保留；无全局/tls串实例。
- 五算法阶段+public process_total计时准确，wait/IO分离；状态缺失不填0。
- 三audit迁移、同schema/serializer/stats；geometry工具确实存在且无新判据。
- App baseline/debug完整离线帧集相同，诊断采样不跳算法帧；视频1676帧非回归0有效输出差异。
- empty无历史有效图形，unknown不回填，历史文本HISTORY隔离，原图不改。
- YAML加载/校验/导出/测试全链，既有算法数值与模型hash未改；未实现视频导出启用明确报错。
- 性能报告真实可追溯，离线不强制实时字段；14ms来源和未达情况清楚。
- README、docs/INDEX、tools/INDEX、diagnostics_schema、block5_acceptance、evidence导航与完整manifest齐全。
- 本轮所有证据归档后核验，可从归档重放验证；本任务固定build经检查清理，失败证据也保留。

14ms达标、实拍准确率、一般透视/远距离覆盖、H新网格、多目标/置信度不是本板块完成条件。建议目标是尽量低的观察开销，必须报告真实代价，不能以建议代替硬项。

### 10.2 固定目录与可复放证据

固定构建：仓库根 `build/block5-baseline/`（未改基线）、`build/block5/`（Release）、`build/block5-debug/`（相关Debug）。如需要fixture，唯一 `build/block5-fixture/`；不使用temp2/final3等散目录。

**运行证据从一开始直接写入：**

```text
src/tushenghao/docs/evidence/block5/
  INDEX.md
  step0/                  # baseline、RouteB反例、前后结果、patched manifest
  tests/                  # configure/build/CTest/主动用例，失败也编号保存
  runs/<唯一run_id>/       # manifest/effective_config/summary/frames/side表/evidence
  comparison/             # 逐帧差异、失败索引、schema与归档核查
  environment/            # toolchain、安装依赖、source/code/model/input指纹
  archive/                # artifact-manifest.json、commands.json、完整性核查
```

不将必需脚本写build后仅在文档提一条路径。重放工具源码放tools/validation；证据中的脚本若确需保留放block5目录并登记。manifest每个产物写相对路径、大小、SHA256、生成命令/退出码、条件、来源版本；指纹生成沿已有tool能力，不要求安装新依赖。

artifact-manifest不包含自身的自引用hash；由独立归档核查记录其最终hash。baseline/patched/final三类source hash分别记录，不能用同一个外部提交标签替代；旧baseline可执行程序在改代码前构建并运行，后续禁止重新configure它后仍叫旧基线。

固定构建命令（仓库根，日志写上述证据目录）：

```bash
cmake -S src/tushenghao -B build/block5-baseline -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=b9cccd4
cmake --build build/block5-baseline -j 4
ctest --test-dir build/block5-baseline --output-on-failure
# 保存原基线结果后才改代码；后续新代码只使用以下目录。
cmake -S src/tushenghao -B build/block5 -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=b9cccd4
cmake --build build/block5 -j 4
ctest --test-dir build/block5 --output-on-failure
cmake -S src/tushenghao -B build/block5-debug -DCMAKE_BUILD_TYPE=Debug -DMARK_COMMIT_LABEL=b9cccd4
cmake --build build/block5-debug -j 4
ctest --test-dir build/block5-debug --output-on-failure
```

`observability_verify`确定接口：`--compare <基线run> <本次run>`、`--check-run <run目录>`、`--check-archive <block5证据目录>`，每次另给`--report <新JSON文件>`；未知/重复选项、已有report、缺输入非零。compare输出逐帧差异/失败索引及明确排除字段；Step0增加`--route-b`只允许本文批准的置换类别，并验证恢复物理float点一致。check-run核schema/计时状态/配置/实际帧数/side表/指纹；check-archive核所有manifest路径、hash、导航和build唯一副本依赖。工具复用已有OpenSSL仅计算证据hash，不链接进Detector；新增reader以独立test/tool支持目标连接，不把tools头目录公开给普通消费者。

Step0时旧App尚不具备完整runner，前/后视频使用已有temporal_audit，一次decode+finalize输出旧格式；明确标SHARED_STAGE_ASSEMBLY，不把它叫App baseline计时。Step0补丁后的旧格式记录即后续算法非回归金样。verify增加**只读历史适配**：检测记录格式后读取旧temporal JSON的`decode`、`finalized`、`temporal`载荷，映射到相同比较结构；不写回、不给旧记录补造stage耗时或重置事件。新scope full与旧scope temporal只比较双方实际共同执行的结果/证据，meta/scope/计时单独核查。patched金样指纹由verify从这份旧记录的finalized结果计算，App最终baseline/debug按同一确定规则计算；不能要求Step0时尚不存在的新runner产出正式性能基线。

Step0旧基线二进制必要时作为不可重建来源单独归档并标平台；普通新二进制不入证据，保存源码hash、构建命令、环境即可重建。大JSONL可无损压缩，但必须保留校验工具可读取方式/原始hash；不能只保留摘要替代逐帧。

清理顺序：所有必要产物进入block5 → 检查每条文档链接/命令不依赖build里的唯一副本 → 实际解析JSONL和校验manifest → 运行归档核查PASS → 验证上述目录绝对路径在当前仓库build内、任务标识匹配 → 只删除这四个本任务build目录。Windows用LiteralPath原生删除，不跨shell组合命令；不删Block3/4 build、输入视频、旧归档或用户未标识目录。

若前序失败未完成：先归档失败/剩余项，再报告；不以清理失败现场换成功。用户已有未知build目录或磁盘不足走G-RESOURCE，不擅自删除旧材料。

### 10.3 中文交付文档

`docs/block5_acceptance.md` 四段式“完成内容/验证方法/踩坑记录/已知限制”，并包含：D01/D02修复内容、用户决议、实际验证表、性能结果、真实遗留问题、主动耗时/剩余项。没有证据的行写未验证，不写完成。

README必须给：依赖/构建、check-config/baseline/debug/三个audit/verify命令、配置层级、输入路径、输出run位置、错误查哪、时钟/14ms口径、历史隔离、限制、归档清理。用户应能独立跑通，不再问输入放哪里。新增补充文档必须登记docs/INDEX，不散放。

## 11. 风险与唯一升级关卡

现有审批均已落实，**没有常规数值待批阻塞**。以下是仅在条件发生时停下的异常关卡，不能当成Codex自行取舍的权限。

| G/风险 | 触发 | 处理 |
|---|---|---|
| G-BASELINE | 不是指定分支/起点，代码有无法解释的先行改动 | 停，提供读到的HEAD/ref/源hash，用户处理Git |
| G-UNEXPECTED | Step0超批准差异；视频不一致；公共布局/签名必须改才能实施 | 保存最小反例/差异，停受影响工作，用户批准；不放宽阈值 |
| G-RESOURCE | 输入缺失/hash不符、既有固定目录无任务标识、磁盘/权限/GUI导致验证不可做 | 归档已完成项，报告具体阻塞；不删旧证据，不偷偷抽样或忽略功能 |
| D23/D25遗留 | 原三L面积规则/角边硬绑定仍存在 | 记已知限制，Block5只观察不重写Block2 |
| r支持不足 | 已批准2px有限条件之外出现对应不确定 | 原始回退，不填新预算；报告失效条件 |
| 长trace成本 | 原日志逐假设格式化成本明显 | 按本稿诊断分级隔离并如实测；不优化几何、隐藏内部成本 |
| IO/metadata成本 | 写盘/指纹影响总吞吐 | 报export/startup/run开销，不混process_total或装成曝光延迟 |

发生不可完成项不得写“Block5完成”。用户决定范围/时间；Codex只报告证据与剩余动作。

## 12. 可直接贴给 Codex 的执行提示词

> 你是实施方，本任务只实现本文件批准的Block5及Step0 RouteB，不负责重新设计算法。执行分支feat/observability，起点b9cccd4a86ac5959a8c67559cde1d399ba29e046。用户做review gate，你不操作Git。
>
> 第0步先直接读.git/HEAD、refs（兼容gitdir/commondir/packed-refs）确认基线和分支，记录sourcehash；不调用任何Git命令，也不让构建系统间接调用。逐一阅读4.md、5.md、v0.8.1_Detector.md及Block4终稿和docs/evidence/block4，保持冻结原文只读。
>
> 按本方案§2先建立未改基线、做D01 RouteB：Block3在原证据验证之后统一float规范排序，保持物理标签、同一坐标及正确orientation，不改模型/阈值/证据。补忠实于生产顺序的分界用例，跑19/19和同hash视频1676帧逐帧对照。允许的表示置换逐项记账；状态/数量或其他非批准差异走G-UNEXPECTED，不自判“更好了”。补丁基线通过才能推进。
>
> 按§8 Step1–8完成：六模块内FrameRecord显式实例通道、D02真实reset事件、阶段插点和public-call total、统一记录/统计/三audit、App离线baseline/debug、绘制和YAML全链、回归与归档。公共四函数签名和Detection/FrameResult/DisplayState布局不变，保留vector<string> diagnostics；friend仅私有内部访问，不加公开setter，不用全局/tls。具体文件/接口/状态/边界按§3–7，不自行挑实现路线。
>
> 所有实质新增/修改函数和类定义处写清晰中文注释，说明“原来什么问题、为什么这样改”，不逐行翻译、不只写优化/修复。计时/诊断/渲染参数YAML化，同步严格加载/校验/导出/测试；算法数值原样保留。基线不跳帧、不关稳定、不删慢帧，debug采样只控制记录。历史只文字HISTORY，empty无有效框、unknown不回填。
>
> 禁止修改冻结ref原文、v0.1、几何/取证/语义核心算法或阈值；禁止新置信度/Kalman/预测/多目标/持久ID、新测试框架或新第三方库；禁止删测试换绿、只看detections≥1/CTest数量就宣称完成、从视频反推门限、编造未运行PASS。非有限序列化null+原因，未执行耗时不填0，三audit不得保留第二套JSON标准。
>
> 执行§9每组主动断言，所有旧覆盖保留；最终Release预期23/23、相关Debug、全视频0–1675非回归、scope/schema/IO失败与归档核查均需实际证据。性能真实报告，对照14ms只看公共process_total；不达不授权优化。用户旧批准r2/偏离2只读，不重新审批。
>
> 本Block所有证据直接归档src/tushenghao/docs/evidence/block5/；构建仅固定四目录，每次失败也保存日志。完成归档hash/链接/重放核查后，检查绝对目标和任务标识，仅清本任务build，不动旧build/视频/归档。文档/脚本不得依赖被删目录里的唯一副本。
>
> 交付中文docs/block5_acceptance.md、docs/diagnostics_schema.md、README/全部INDEX和完整证据manifest/commands，记录修复内容、验证结果、遗留问题、实际耗时与剩余项。缺任一强制项写未完成并提供阻塞，不替用户删范围。出现§11关卡保存具体证据后停受影响工作，向用户报告，其他独立工作可继续。
>
> 最后给用户四行摘要：①实际修改与RouteB基线；②实际运行及通过/失败；③真实性能与证据路径；④遗留/关卡与review所需动作。不要执行Git或自行合入main。
