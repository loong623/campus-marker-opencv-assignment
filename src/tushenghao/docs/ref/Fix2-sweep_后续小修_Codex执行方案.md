# Fix2-sweep 后续小修：Codex 执行方案

## 1. 目标、基线与边界

本轮修正两个测量/诊断问题，补充 README 快速开始，并单独整理代码排版。**不提高检出率、不重定预算、不改变现有正常输入的检测决策。**

审阅基线：`feat/final-fixes`，`bab614ac74b2a3ddc54097cb912a1d2dfd028a5c`。以下行号对应该快照；排版后以函数名、字段名定位，不机械套用旧行号。

已实际读取该分支的源文件、配置、测试及相关文档。此次为静态审阅与实施方案，**尚未替用户执行编译、25 项测试或视频回归**，验收结果须由 Codex 实测填写。

依据：

- [fix2_sweep_report.md，锁定审阅快照](https://github.com/shenghaotu81-droid/campus-marker-opencv-assignment/blob/bab614ac74b2a3ddc54097cb912a1d2dfd028a5c/src/tushenghao/docs/fix2_sweep_report.md)：当前快照的计数问题在约 L211，几何残差问题在约 L259；与用户先前引用的行号不同，结论一致。
- [measure.cpp](https://github.com/shenghaotu81-droid/campus-marker-opencv-assignment/blob/bab614ac74b2a3ddc54097cb912a1d2dfd028a5c/src/tushenghao/tools/block3_fixture/calibration/measure.cpp)：L154。
- [geometry_validation.cpp](https://github.com/shenghaotu81-droid/campus-marker-opencv-assignment/blob/bab614ac74b2a3ddc54097cb912a1d2dfd028a5c/src/tushenghao/lib/geometry/geometry_validation.cpp)：`polygonResidual`、`checkGeometricConsistency`、`validateGeometryBatch`。
- `src/tushenghao/docs/ref/`：冻结契约只读；不得改写原文。

### 1.1 不准做

1. 不改 `max_line_fit_error=0.5`、`max_edge_position_distance_px=9`，也不改其他阈值、默认值、配置加载或校验边界。
2. `0.75/9.5` 已撤回；不得重新推荐、写入配置或用于验收。
3. 不重构成像模型，不新增模糊/光照模型来重新推导预算；该工作需要另立控制实验项目。
4. 不对支持弧先去重再拟合；不改 `cv::fitLine` 输入、权重、残差分母、转折裁剪和边绑定。
5. 不修改公共函数签名、`Detection`/`FrameResult` 布局；不实现置信度、补点、预测或历史框桥接。
6. 不借排版改排序、循环顺序、比较运算符、浮点类型、容差或候选截断。
7. 不修改旧证据、旧实验输出或冻结文档；不执行 Git 操作。

## 2. 审阅结论与修复口径

| 项目 | 位置 | 已确认问题 | 本轮处理 |
|---|---|---|---|
| F01 | `tools/block3_fixture/calibration/measure.cpp:154` | `support_points` 写的是弧的访问次数，回走像素被重复计数 | 改为每条弧独立的唯一像素坐标数；原始弧完整保留 |
| F01-C | `tools/block3_fixture/reporting/report_verification.py:67` | 独立验收脚本以 `len(arc)` 核查最少支持点，与生产去重计数不一致 | 同源计数口径补丁；原有 `10` 门槛不变，不改其他验收规则 |
| F02 | `lib/geometry/geometry_matcher.cpp:367`；`lib/geometry/geometry_validation.cpp:156–205,335–339` | 生成器写占位 0，验证器已计算真实距离但没有回填 | 在验证层回填现有距离统计；未计算与真实 0 必须区分 |
| F03 | `README.md` | 当前操作入口埋在历史说明里 | README 改为快速操作入口；历史全文迁入 `docs/history/` 并链接，不丢内容 |
| F04 | 多个内部实现、测试、工具 | 单行多语句、长函数行等影响阅读；如 `run_report.cpp`、`decode_stage.cpp`、部分 audit 工具 | 最后独立排版，逐文件证明未改变代码逻辑 |

这里的 F01-C 是 F01 的下游口径修正，不是另做检测算法。生产 `corner_edge_fit.cpp` 已用集合统计支持点，`corner_evidence_validation.cpp` 也按去重后的点数核查；这两处计数逻辑保持原样。

静态审阅的数据流为：预处理 → 白块/简化观测 → 锚点与仿射假设 → 几何验证 → M/S 对应补全 → 原图四点取证 → 语义与屏幕排序 → 当前检测 → 时序稳定 → 记录/导出。未据此证明全系统没有其他问题；本轮不把推测扩大成生产逻辑修改。

## 3. 文件修改清单

下列路径均相对仓库根目录。

| 文件 | 操作 | 精确职责 |
|---|---|---|
| `src/tushenghao/tools/block3_fixture/common/support_pixel_count.hpp` | 新增 | 工具专用、有限范围的唯一支持像素计数 helper；不得成为通用 utils |
| `src/tushenghao/tools/block3_fixture/calibration/measure.cpp` | 修改 | F01；调用计数 helper，保留原始访问次数和弧 |
| `src/tushenghao/tools/block3_fixture/reporting/report_verification.py` | 修改 | F01-C；独立地按唯一坐标计数，维持独立验收性质 |
| `src/tushenghao/tools/block3_fixture/CMakeLists.txt` | 修改 | 把新增计数头文件纳入测量代码哈希输入；不得漏掉这次真正影响指标的代码 |
| `src/tushenghao/lib/geometry/geometry_validation.cpp` | 修改 | F02；返回并回填验证层实际残差，不新增判据 |
| `src/tushenghao/lib/geometry/geometry_matcher.cpp` | 极窄修改 | 仅将生成阶段的占位赋值改为明确的未计算标记及注释；不得动生成/锚点/竞争逻辑 |
| `src/tushenghao/lib/core/geometry_types.hpp` | 仅说明注释及排版 | 解释字段单位、统计口径、阶段语义；不改布局及默认初始化 |
| `src/tushenghao/tests/corner_edge_fit_test.cpp` | 修改 | 增补支持计数主动断言，生产弧拟合行为不变 |
| `src/tushenghao/tests/geometry_matcher_test.cpp` | 修改 | 增补验证残差主动断言；不重写已有 fixture，不删旧用例 |
| `src/tushenghao/CMakeLists.txt` | 极窄修改 | 为 `corner_edge_fit_test` 增加工具 helper 的私有 include 路径；25 个 CTest 目标保持原名原数量 |
| `src/tushenghao/.clang-format` | 新增 | 仅开发期排版规则，不增加生产运行依赖 |
| `src/tushenghao/README.md` | 整理 | 三步快速开始、三种运行模式、输出说明和历史链接 |
| `src/tushenghao/docs/history/README_before_fix2_followup.md` | 新增 | 完整保留原 README 内容；仅修正迁移后的相对链接并加历史说明 |
| `src/tushenghao/docs/fix2_followup_log.md` | 新增 | 施工过程、实际耗时、每阶段结果和剩余项 |
| `src/tushenghao/docs/fix2_followup_acceptance.md` | 新增 | 本轮验收表、测量前后对照、回归结果、限制与证据链接 |
| `src/tushenghao/docs/INDEX.md` | 修改 | 登记两个新文档，不删历史导航 |
| `src/tushenghao/docs/fix2_sweep_report.md` | 追加 | 本轮后续小修状态和验收链接；不改旧调查、旧数值或撤回结论 |
| `src/tushenghao/docs/final-fixes_acceptance.md` | 追加 | 记录 follow-up 的范围与结果，链接独立验收报告；历史验收不覆盖 |

其他 C++ 文件只能进入第 7 节的排版范围；不允许功能性修改。若发现其他真实逻辑问题，记录位置、证据和建议，交用户另行决策。

## 4. F01：支持点数量

### 4.1 定义与签名

工具 helper 的签名固定为：

```cpp
std::size_t count_unique_support_pixels(
    const std::vector<cv::Point2d>& support_arc);
```

放在工具专用命名空间内，不导出为 `mark` 公共接口。

计数规则：

- 对一条弧的 `(x,y)` 坐标对精确去重；空弧返回 0。
- 与生产当前 `std::set<std::pair<double,double>>` 口径一致；不四舍五入、不添加 epsilon、不按相邻点去重。
- 两条弧分别计数，不把两条弧合并去重。
- 拒绝非有限坐标，报测量输入错误；不得把 NaN 放入排序集合，不得静默跳过。
- helper 不修改传入弧。

当前支持弧来自原图轮廓像素；`cv::Point2d` 是存储类型，不表示这里可以合并不同亚像素坐标。未来若支持点来源变化，须重新确认计数语义，不在本轮提前扩展。

### 4.2 measure.cpp 改动

在当前 `corners.push_back` 中：

| JSON 字段 | 新含义 |
|---|---|
| `support_points` | 长度为 2，分别为两条支持弧的唯一像素坐标数量 |
| `support_visits` | 新增、长度为 2，保存原 `.size()`，便于核对回走次数 |
| `support_points_counting` | 新增，固定字符串 `unique_pixel_coordinates` |
| `support_arcs` | 完整保留原顺序、重复点与所有坐标 |

其他字段，特别是 `line_mean_residual_px`、`line_max_residual_px`、角点、延伸距离、真值距离，计算方式全部不动。新增字段只属于标定工具 JSON，不更改公共检测布局或生产 FrameRecord schema。

函数/关键输出处用中文解释：旧字段把轮廓回走次数当成独立支持点；修正计数使其与生产最少支持点判据一致；拟合仍使用完整访问序列，避免改变原权重和 0.5px 定义。

`CMakeLists.txt` 的代码哈希输入必须显式包含新增 helper。当前 `MEASURE_TOOL_SOURCES` 只 glob `*.cpp`，不能指望它自动覆盖新增 `*.hpp`。

### 4.3 下游脚本

`report_verification.py` 内新增小型纯计数函数：对一条 JSON 弧的二维坐标对去重计数；将 `any(len(arc)<10 ...)` 换成相同门槛下的唯一点计数。保留四角、方向、bbox、H 分层门槛和报告格式。

不要依赖测量输出的 `support_points` 来证明它自身正确；脚本仍从 `support_arcs` 独立计算。`summarize.py` 已消费 `support_points`，无需改其公式。本轮不运行预算写回流程。

### 4.4 必测

在现有 `corner_edge_fit_test` 中加入 Release 下也生效的主动检查：

| 输入 | 预期 |
|---|---|
| 空弧 | 0 |
| 3 个不同坐标 | 3 |
| 同一坐标访问 5 次 | 1，访问序列不变 |
| 非相邻回走序列，6 次访问、3 个不同坐标 | 3 |
| 两条各有重复的弧 | 分别计数，不能跨弧合并 |
| 两个不同的有限坐标 | 不得用 epsilon 合并 |
| 含 NaN 或无穷 | 明确报错 |

对 Python 独立计数函数运行标准库级小测试，至少覆盖空弧、回走重复和边界：10 次访问但仅 9 个唯一点必须不满足原 10 点门槛，10 个唯一点满足。使用临时只读测试驱动即可，不引入新框架。

重新运行少量 C fixture 的测量，核对每条弧 `support_points == 唯一坐标数`、`support_visits == 原弧长度`。对无重复弧应计数不变；不改变原拟合/角点指标。不要求重造整个 C/H 网格来证明一个计数函数，也不重新标定预算。

报告已记载 C-v5 的 5760 条边没有重复像素，因此不能声称本轮计数修复推翻历史 0.5px 推导。实拍的重复支持点案例用于说明旧计数为何误导，不用于推高预算。

## 5. F02：真实几何验证残差

### 5.1 使用现有定义，不发明新指标

设当前假设对应部件集合为 `J`；对部件 `j` 的投影模型顶点集合 `Pj`：

```text
rj = mean(abs(pointPolygonTest(观测完整轮廓j, 投影模型顶点p, true)))
validation_residual = max(rj，j 属于 J)
```

**单位：工作图像像素。** 最大值在部件之间取，均值在该部件模型顶点之间取。它对应现有逐部件 `residual > max_validation_residual_` 的验证逻辑。

不是所有部件顶点混在一起求平均，不是三个锚点自拟合误差，不是线拟合残差，不是原图误差，也不是预测不确定度。三 L 验证后再补 M/S 时，该值仍是父假设本次已验证部件的统计，不能声称已经量测六个部件。

### 5.2 改动位置与顺序

1. `buildGeometryHypothesis`：仅把未计算的赋值改为 `-1.0`，加中文注释说明原 0 会被误读为完美拟合；`-1` 是状态标记，不是预算。不得修改类型默认值、公开签名、候选生成和仿射求解。
2. `checkGeometricConsistency`：保持其私有性质，末尾增加 `double& measured_residual` 输出参数。逐部件调用原 `polygonResidual`，以局部最大值累积；已有超预算拒绝条件保持 **`>`**，不改为 `>=`。
3. 只有完成现有几何一致性检查后，输出完整、有限且非负的统计。缺模型/缺组件等原有失败路径继续失败；没有完整测量不得伪造 0。空对应集合、非有限结果属于不可计算输入，应明确拒绝并测试，不得额外引入新的正常图像判据。
4. `validateGeometryBatch`：继续复制输入为 `checked`，验证成功后立即把统计写入 `checked.validation_residual_`，然后执行原面积比和完整性处理。输入 batch 不变。
5. M/S 补全、decode 和记录层继续读取该字段；不得用它重新排序、选唯一解释或改变置信度。既有 `geometry_assignment_completion.cpp` 的有限且非负检查保持原样。
6. 不另写一套“诊断专用距离公式”。生产验证处算一次并回填，让现有 trace/序列化读到它。

私有 helper 签名可调整，以下公开签名不变：

```cpp
GeometryBatch validateGeometryBatch(
    const GeometryBatch& batch,
    const MarkerGeometry& geometry,
    const std::vector<WhiteComponent>& components,
    const GeometryConfig& config);
```

`geometry_validation.hpp` 中“Step6 不修改”的原职责约束继续有效：本轮获准的生成器改动仅限该诊断字段占位语义，不能借此重写 Step6。冻结 `ref/` 原文不改。

### 5.3 必测

在现有 `geometry_matcher_test` 中增补主动检查，不能只用会在 Release 消失的 `assert`。

| 用例 | 输入/构造 | 必须断言 |
|---|---|---|
| G01 | 有效仿射、模型顶点恰在观测轮廓上 | 验证后残差确为 0，而不是占位值 |
| G02 | 测试模型用方形；观测轮廓向四侧各扩 1 工作像素 | 每个模型顶点距轮廓 1，字段为 1 |
| G03 | 三个部件的观测轮廓分别外扩 0、1、3 工作像素 | 字段为 3，不能为 `4/3`；fixture 阈值须允许本用例，不改生产配置 |
| G04 | 同一几何，测试阈值恰为实际残差，然后取其下一个更小浮点值 | 相等通过；略小拒绝；原比较边界保持 |
| G05 | 输入 hypothesis 为未计算状态 | 输入保持不变；返回副本为真实统计 |
| G06 | 有效统计之后走原面积比/完整性分支 | 若保留该假设，其字段仍为本次实际统计，不回到 0 |
| G07 | 缺组件、缺模型、空对应、非有限结果 | 不产生带“真实 0”的有效输出，不崩溃，不猜统计 |
| G08 | 模型对应使用非连续 component ID | 按 ID 查组件，不能把 ID 当数组下标 |

G02/G03 是测量函数 fixture，不是替换生产 MARK 模型。浮点比对按本测试计算精度设置严格数值检查；禁止因此增改生产 epsilon。

采集一小组有效假设，将字段与独立 `pointPolygonTest` 计算结果逐项对照。保存各部件均值和最大值；任何不一致先修实现，不改预算。

## 6. README：三步快速开始与历史迁移

融合用户追加的 P1 七项要求。保留“三步”结构：装依赖与构建 → 检查配置与选择运行模式 → 看结果。README 不再夹杂各 Block 的历史操作全文，历史完整保存在单独文档里，入口只留链接。

先将修改前 README 原始字节归档到 `docs/evidence/final-fixes/fix2-followup/readme/README-before.md`；再建立 `docs/history/README_before_fix2_followup.md` 保存原全文。历史文档开头注明“历史记录，当前操作见项目 README；旧验收数字不代表本轮结果”。不删 Block3/4/5、Final-fixes、Path A 的段落或代码块。

迁移后的相对文件链接按新位置修正：原相对 `src/tushenghao/` 的目标，从 `docs/history/` 出发应先回到 `../../`；绝对 URL 和页内锚点不改。保留原命令文本，注明历史命令仍按仓库根目录理解，不建议新人直接执行。将链接改动单独记录，不能借修链接改历史事实。

整理后的 README 只保留：简短项目说明、快速开始、必要的路径/输出说明、已知限制简述、文档导航。已知限制用当前报告链接解释，不把历史检测数量写成当前正确率。

### 6.1 第一步：装依赖并构建

说明 Ubuntu/Linux，从仓库根目录执行。每个命令单行，不用反斜杠续行。

```bash
sudo apt-get update && sudo apt-get install --no-install-recommends build-essential cmake libopencv-dev libssl-dev
cmake -S src/tushenghao -B build/release -DCMAKE_BUILD_TYPE=Release && cmake --build build/release -j4
```

标定工具额外使用的 `nlohmann-json3-dev` 放工具说明中，不让普通运行者误以为必须先构建 fixture 工具。`clang-format` 是开发工具，也不是运行依赖。

### 6.2 第二步：检查配置并跑通

```bash
VIDEO="<你的视频绝对路径>"
build/release/marker_app --check-config --config src/tushenghao/config/detector_verification.yaml
build/release/marker_app --check-config --config src/tushenghao/config/detector.yaml
build/release/marker_app --check-config --config src/tushenghao/config/detector_debug.yaml
```

然后列出三种运行方式，每种一句用途和一条单行命令。用户选择其一，不要求第一次把三种全跑完。

| 模式 | 用途 | 输出目录 |
|---|---|---|
| baseline | 无弹窗的处理与汇总 | `new-runs/quickstart-baseline-01` |
| debug + display | 看检测框与逐帧诊断 | `new-runs/quickstart-debug-01` |
| verification + export-video | 保留逐帧证据和带框视频 | `new-runs/quickstart-verification-01` |

```bash
build/release/marker_app --video "$VIDEO" --config src/tushenghao/config/detector.yaml --mode baseline --run-dir new-runs/quickstart-baseline-01
build/release/marker_app --video "$VIDEO" --config src/tushenghao/config/detector_debug.yaml --mode debug --display --run-dir new-runs/quickstart-debug-01
build/release/marker_app --video "$VIDEO" --config src/tushenghao/config/detector_verification.yaml --mode debug --run-purpose verification --export-video --run-dir new-runs/quickstart-verification-01
```

说明占位符必须替换，路径含空格时保留双引号；输出目录必须是新目录，再跑把对应后缀改为 `02` 等，不覆盖证据。所有快速开始命令禁止反斜杠续行。

`--display` 用于有图形桌面的 debug；无图形环境移除该选项或选择视频导出，以文件查看，不要求弹窗。verification 是报告用途，CLI 仍使用 `--mode debug --run-purpose verification`，不得编造 `--mode verification`。

`--expected-frames N` 是已知帧数的截断核验参数，不是通用固定值。快速开始不写死 1676；本项目已知视频回归可显式写 1676。省略时使用可用的容器帧数；无法可靠核验时，以实际报告提示说明“完整性未验证”，不承诺一定判成功。

### 6.3 第三步：看结果

| 输出 | 用途 |
|---|---|
| 所选 `--run-dir` 下的 `summary.yaml` | 帧数、处理时间、状态汇总；不是 `summary.json` |
| 同目录 `frames.jsonl` | debug/verification 的逐帧状态与诊断；不要承诺 baseline 也有详情 |
| 同目录 `effective_config.yaml`、`manifest.yaml` | 本次配置、输入与运行来源 |
| 同目录 `overlay.mp4` | 带框视频，只有启用导出时才存在 |

解释终端保留技术 ID，中文摘要用于快速查看帧数、耗时和瓶颈。导出视频采用现有 MJPG in MP4；部分 VS Code 内置播放器不支持，使用系统播放器或安装 FFmpeg 后：

```bash
ffplay new-runs/quickstart-verification-01/overlay.mp4
```

本轮不改编码或播放功能。

**历史导航：**README 链接到 `docs/history/README_before_fix2_followup.md`，再链接 `docs/INDEX.md`、当前 `docs/fix2_sweep_report.md`、本轮 `docs/fix2_followup_acceptance.md`。INDEX 同步登记历史文档。

**保留证明：**原字节快照哈希一致；迁移文档逐段比对，除了新增历史提示和必要的相对链接调整，原正文、数值、段落、代码块全部保留。链接目标逐个验证存在。新入口全部使用 `$VIDEO`，旧示例只在标注为历史的文档中保留。

## 7. 可读性：单独阶段，禁止夹带逻辑改动

### 7.1 规则

- Allman 大括号、4 空格缩进；尽量控制在 100 列。
- 函数定义之间留一个空行；初始化、校验、计算、输出等逻辑段之间留一个空行。
- 不写单行函数体或单行多个控制块；条件、循环、返回清楚分开。
- 不逐行翻译代码；实质修复在函数或关键职责处写中文“原来什么问题、为何这样改”。
- 不重命名冻结签名。不因函数当前 CamelCase 就改成 snake_case；只记录命名遗留。
- 不改变字符串、日志字段、数学表达式、include 顺序或宏指令语义。截图中的 `1e-9` 只是排版示例，不是新增数值授权。
- 本轮不拆大函数或搬模块；若必要，另列建议。排版做到逻辑段可读即可，不把语义重构伪装成格式化。

新增 `.clang-format` 至少确定：LLVM 基础、`IndentWidth: 4`、`ColumnLimit: 100`、`BreakBeforeBraces: Allman`、禁短函数/短 if/短循环、`MaxEmptyLinesToKeep: 1`、`SortIncludes: false`、`ReflowComments: false`、`BreakStringLiterals: false`。用实际安装版本验证配置，记录版本；不能更改字符串来硬凑列宽。

### 7.2 允许排版的范围

仅 `src/tushenghao/` 自有 C++ 源文件：

| 目录 | 关注点 |
|---|---|
| `lib/config/`、`lib/core/` | 配置大段、计时器、类型说明 |
| `lib/preprocess/`、`lib/geometry/` | 提取→生成→验证→补全，阶段边界可读 |
| `lib/corners/` | 支持弧、拟合、取证与验证分段 |
| `lib/pipeline/` | decode、稳定、诊断、报告；重点长行和同一行多步骤 |
| `app/` | 参数解析、运行/显示/导出流程 |
| `tests/`、`test_support/` | Arrange/Act/Check 分段，已有断言不变 |
| `tools/audit/`、`tools/common/`、`tools/validation/` | CLI、解析、比较与报告分段 |
| `tools/block3_fixture/` 的 C++ 源文件 | 生成、标定、诊断用途清楚，不重写生成器 |

不格式化 `include/mark/` 公共头文件、`tools/synth_ref/`、第三方、`docs/evidence/`、`docs/ref/`、配置和构建产物。Python 脚本仅处理 F01-C，不全仓库重新排版。

先列出实际要整理的文件及原因，保存 `style/files.txt`；只对清单执行。已经清楚的文件不制造无意义 diff。

### 7.3 防夹带检查

功能修复测试通过后保存“排版前”源码快照或逐文件哈希清单。排版后比较 C++ token 序列：除注释和普通空白外必须一致；保留字面量、运算符、标识符及预处理指令换行语义。不得用简单去掉所有空格的方法比较，它会误伤字符串和宏。

使用编译器词法工具或可靠的临时检查驱动，归档版本、命令、结果。不能证明的文件逐个审 diff；不批准“看起来没变”代替检查。排版阶段必须再次跑 Release/Debug 25 项测试。

## 8. 执行顺序与检查点

| 步骤 | 做什么 | 通过后才进入下一步 |
|---|---|---|
| 0 | 确认用户提供工作树、分支信息及基线；检查现有修改、输入、固定 build 目录；保存配置/冻结文档哈希 | 无冲突；不是让 Codex 切分支或运行 Git。若基线不同，先比较相关源文件并记录，不覆盖用户修改 |
| 1 | 编译基线、确认 25 项测试；生成或核实同视频同配置基线记录 | 测试及视频完整性可核查；若缺原视频或旧基线不匹配，写阻塞，不拿别的视频数量代替 |
| 2 | F01、F01-C、代码哈希覆盖及支持计数测试 | 唯一点数正确；拟合访问序列与旧指标不变 |
| 3 | F02 及 G01–G08 | 真残差正确、原验证边界不变、未计算不冒充真实 0 |
| 4 | 跑功能修改后的 Release/Debug 25 项，整理 README 和迁移历史、登记文档 | 双配置均 25/25；历史完整保留，迁移链接正确 |
| 5 | 仅执行清单内排版，检查 token，再跑双配置测试 | 无逻辑 token 改动；双配置仍 25/25 |
| 6 | 最终全视频回归、README 新人路径实操、证据归档 | 下列验收表逐项满足或明确标记未完成 |
| 7 | 写完成文档、检查链接和证据、清理本轮 build | 文档完整；证据与可复现说明仍存在 |

遇到意外检测变化、阈值变动或新逻辑问题，停在该检查点，提交证据与最窄建议；不得自行扩展到 fix2 成像模型或新预算实验。

## 9. 验证命令与最终验收

所有命令从仓库根目录执行；示例为 Linux。每条命令单行。输出目录必须不存在；已存在则核对是否本轮所有，选择新的证据子目录，不删除旧证据。

### 9.1 构建与测试

```bash
mkdir -p src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests
cmake -S src/tushenghao -B build/fix2-followup-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/fix2-followup-release -j4
ctest --test-dir build/fix2-followup-release --output-on-failure --output-log src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests/release.log
cmake -S src/tushenghao -B build/fix2-followup-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/fix2-followup-debug -j4
ctest --test-dir build/fix2-followup-debug --output-on-failure --output-log src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests/debug.log
```

主动检查必须在 Release 生效。保持 25 个 CTest 目标，不以删旧测试、隐藏失败、改旧门槛换绿。功能阶段和排版阶段日志分别用不同文件名，不能覆盖。

标定工具：

```bash
sudo apt-get install --no-install-recommends nlohmann-json3-dev
cmake -S src/tushenghao/tools/block3_fixture -B build/fix2-followup-fixture -DCMAKE_BUILD_TYPE=Release
cmake --build build/fix2-followup-fixture --target block3_fixture block3_measure -j4
build/fix2-followup-fixture/block3_fixture sample C src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/C-sample
build/fix2-followup-fixture/block3_measure src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/C-sample src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/measurement-after.jsonl
```

提前创建 `calibration/` 父目录。`sample C` 复用现有生成器，仅生成小样；不新建渲染模型。修改前运行同样小样得到 `measurement-before.jsonl`；逐 case 比较计数及非计数定位字段。重复点的确定性单测是必须项，不能因为小样没有重复就省略。

### 9.2 配置与全视频

```bash
VIDEO="<已确认的原始1676帧视频绝对路径>"
build/fix2-followup-release/marker_app --check-config --config src/tushenghao/config/detector.yaml
build/fix2-followup-release/marker_app --check-config --config src/tushenghao/config/detector_debug.yaml
build/fix2-followup-release/marker_app --check-config --config src/tushenghao/config/detector_verification.yaml
build/fix2-followup-release/marker_app --video "$VIDEO" --config src/tushenghao/config/detector_verification.yaml --mode debug --run-purpose verification --expected-frames 1676 --run-dir src/tushenghao/docs/evidence/final-fixes/fix2-followup/verification-after
build/fix2-followup-release/observability_verify --check-run src/tushenghao/docs/evidence/final-fixes/fix2-followup/verification-after --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/fix2-followup/check-run.json
build/fix2-followup-release/observability_verify --compare src/tushenghao/docs/evidence/final-fixes/fix2-followup/baseline src/tushenghao/docs/evidence/final-fixes/fix2-followup/verification-after --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/fix2-followup/regression.json
```

`baseline/` 必须是第 1 步修改前的有效完整运行，或经输入/配置/生产源码一致性核对的现有基线。不能拿 Block5 旧 864 帧基线与当前 Path A 比较。报告当前记录的 976 检出/700 empty 仅作核对参考，不是召回率、正确率或本轮完成条件。

现有 `observability_verify` 已排除 `geometry/parent=` 的诊断文本差异，并比较共同测量、时序及结果。不修改比较器过滤规则来掩盖回归。允许真实残差及源码指纹的预期变化；状态、检测四点、方向、bbox、tracks、display、帧时戳和其他拒绝原因必须无意外变化。对诊断改变另作真实残差检查，不能只靠比较器 PASS。

### 9.3 必须完成的验收表

| ID | 验收动作 | 通过条件 |
|---|---|---|
| A01 | 检查源文件和配置哈希 | 配置、冻结 ref、生产阈值全部未改；公共签名/布局未改 |
| A02 | 支持点计数单测及 C 小样前后对照 | F01 全部通过；新字段与原弧可独立复核；拟合/定位非计数指标不变 |
| A03 | 独立脚本计数边界测试 | 重复访问不能凑足门槛；原 10 点门槛及其他验收标准未动 |
| A04 | G01–G08 及真实假设对照 | 字段等于实际最大部件均值；单位明确；未计算不冒充真实 0 |
| A05 | Release 和 Debug CTest | 各 25/25，新增断言在两种构建均执行 |
| A06 | 同输入同配置 1676 帧比较 | 帧号 0–1675、完整性通过、算法载荷零回归；诊断变化限定且有解释 |
| A07 | 从 README 新快速开始实操 | 三步能构建、检查三配置、分别跑三模式并找到对应真实输出；新命令单行、不写死视频路径；帧数和播放器限制解释清楚 |
| A08 | 核对 README 历史迁移 | 原快照哈希一致；历史全文保留，只有提示和链接调整；README 不再堆历史操作，导航可达 |
| A09 | 排版清单与语义检查 | 清单可追溯；排版前后 token 不变；阈值、字符串、宏语义不变 |
| A10 | 文档、证据、清理检查 | 两份新文档、INDEX、追加记录齐全；证据可读；只清理本轮 build |

无视频、无图形桌面或环境安装失败时，标记对应项“未验证”并解释原因；不得把未验证写成通过。完整完成须达到所有适用硬项。

## 10. 记录、归档与清理

### 10.1 两份 Markdown 文档

`docs/fix2_followup_log.md`：

1. 起点、用户授权范围和禁止事项。
2. 实施顺序，实际起止/耗时、每步输出、失败和处理。
3. F01/F01-C/F02 定义，哪些计算明确未改。
4. 排版范围、检查方法和文件清单。
5. 剩余项/新发现；不得宣布解决了闪烁、成像偏差或预算推广问题。

`docs/fix2_followup_acceptance.md`：

1. 结论：完成/未完成及原因，不写空泛“全部优化”。
2. A01–A10 实测表、环境与依赖版本。
3. 支持点“访问数/唯一数”对照、几何残差“各部件均值/最大值/字段值”对照。
4. 25 项双配置及全视频结果；配置/输入/源码哈希。
5. README 三模式实操、P1 七项对账与历史迁移保留证明。
6. 已知限制和证据相对链接。

施工中用“进行中”，所有硬项通过后才改“完成”。链接从 Markdown 所在目录正确解析。

### 10.2 证据目录

```text
src/tushenghao/docs/evidence/final-fixes/fix2-followup/
├── commands.md                 # 实际命令、退出码、环境，不写虚构结果
├── hashes.json                 # 输入、配置、ref、源码、历史块哈希
├── baseline/                   # 修改前有效完整记录
├── tests/                      # 功能阶段/排版阶段 Release 与 Debug 日志
├── calibration/                # 小样 manifest/真值、前后测量、计数核查
├── geometry/                   # 残差对照与测试记录
├── readme/                     # 原 README 字节快照、迁移比对、链接与实操检查
├── style/                      # 文件清单、工具版本、token 检查结果
├── verification-after/         # 最终全视频原始报告
├── check-run.json
└── regression.json
```

不复制原始大视频进仓库；记录路径及哈希。原 fix2-sweep 调查与证据保持原位，不覆盖。

本轮可清理的目录只有：

- `build/fix2-followup-release/`
- `build/fix2-followup-debug/`
- `build/fix2-followup-fixture/`

归档 `Testing/Temporary/LastTest.log`、实际构建日志、CMakeCache 中环境信息和所有需要的实验输出后，核对上述目录确为本轮创建且没有用户资产，再删除。不得清整个 `build/`、其他 Block 目录、README 用户自行建的 `build/release/` 或任何旧证据。若固定目录开工时已有内容，先记录并换本轮专用目录，不擅自清空。

## 11. 可直接交给 Codex 的执行提示词

你负责执行本文件《Fix2-sweep 后续小修：Codex 执行方案》。只改文件，不运行任何 Git 命令；由用户提交、push、merge。

第 0 步核对工作树和用户提供的分支/基线信息，本方案审阅快照为 `feat/final-fixes @ bab614ac74b2a3ddc54097cb912a1d2dfd028a5c`。如实际源文件已变，先核对本方案涉及函数；不覆盖已有修改、不虚构一致性。

按步骤 0→7 执行，先修计数，再修真实残差，完成主动测试；README 做三步快速入口及三种运行命令，历史全文迁入 `docs/history/README_before_fix2_followup.md`，入口只留链接，不丢原内容；最后独立排版并证明没有逻辑 token 改动。F01-C 为独立验收脚本的同源计数修复，不扩展其他验收规则。

所有实质代码改动用中文说明“原来什么问题、为什么这样改”。公共函数签名和布局不变。绝不改变 0.5、9 或其他阈值；不得恢复 0.75/9.5，不重构成像模型，不改拟合输入权重，不以重算预算完成本轮任务。

严格按本方案字段、残差公式、文件归属和测试断言实施。不改冻结 ref，不删测试换绿，不修改回归比较器掩盖差异，不以 `detections >= 1` 或数量一样宣称正确。未批准的其他逻辑修改不能实施。

完成 `docs/fix2_followup_log.md`、`docs/fix2_followup_acceptance.md`，更新 INDEX、README 及两份报告的后续记录。所有证据放到 `docs/evidence/final-fixes/fix2-followup/`。先归档后清理本轮自建 build，保留他人目录与历史证据。

最终汇报：修改文件清单、两个问题的前后对照、Release/Debug 测试、1676 帧算法载荷回归、README 实操、排版语义检查、证据/文档位置、清理结果和未完成项。发现新问题可给位置与分析建议，本轮不得擅自修范围外逻辑。
