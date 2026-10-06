# Final-fixes：13 项修复——Codex 完整执行方案

适用分支：`feat/final-fixes`。基线：`main @ c7fffe4`。本文件供 Codex 实施，用户负责 review、commit、push、merge。

本方案依据基线实际代码、全系统审查报告 R01–R09 和用户追加的四项任务制定。文中行号是 **c7fffe4 的定位锚点**；改动后按函数名定位，不强行套用旧行号。本方案不表示修复已经完成。

## 0. 完成目标与执行边界

13 项按 A → B → C → D 推进。C 单独完成和验证，方便用户检查重构是否改变行为。全部通过后才交付用户做最终 review。

| 批次 | 项目 | 必须交付 |
|---|---|---|
| A | R01、R02、R03、R04、R07、R08、fixture | 失败语义和验证器可靠；配置可迁移；文档可运行；两种构建的旧 fixture 都通过 |
| B | R05、R06 | export 计时遵守开关；geometry audit 不依赖 decode 角点预算 |
| C | R09 | 只格式化、拆内部函数、规范新增内部命名；拆前拆后行为一致 |
| D | display、export-video、中文摘要 | 三项仅接入 App/观测输出；检测算法和公共结果不变 |

硬约束：

1. **Codex 不执行任何 Git 命令**，包括 status、diff、checkout、commit、push、merge。读取 `.git/HEAD` 和引用文件确认用户给定基线、分支允许；由用户处理 Git 操作。
2. 不改 `docs/ref/` 原文；不改历史 `docs/evidence/block3/`、`block4/`、`block5/` 的文件内容或 hash。修改引用这些证据的现行文档允许。
3. 不改 `Detection`、`FrameResult`、`DisplayState` 布局，不改四个公共调用的签名和行为契约。已批准的 `DisplayState.value` 不动。
4. 不改生产阈值、模型坐标、几何匹配与角点定位算法；不做性能优化。只测现有性能，不能声称达到 14ms。
5. 每个实质改动在类/函数定义处写清中文注释：**原来出了什么问题，为什么现在这样改**。不逐行翻译，不用“优化”“修复”替代解释。
6. 保留六个 lib 模块。App 功能放 `app/`，验证工具放 `tools/validation/`；不新增万能 utils，不引入新的测试框架。
7. 不删失败测试、不把测试从 CTest 清单移走、不用 `detections >= 1` 代替验收。
8. 13 项必须有代码/文档落点和验证证据。`docs/final-fixes_acceptance.md`、README、INDEX 缺一不可。

### 不在本轮解决的事项

| 已知未知 | 本轮处理 |
|---|---|
| 812 个空结果是真空还是漏检 | 保持结果；用户合并 fix1 后用 display 目视判断，有问题另开 fix2 |
| 约 94ms 的比赛适用性 | 记录实测，另立性能专项；本轮不优化 |
| 其他损坏输入变体 | 本轮必须覆盖“原视频截断到 262144 字节、解出 41 帧”的复现；不承诺穷尽损坏格式 |

## 1. 第 0 步：确认基线、准备工作区

执行前读取 `.git/HEAD`，确认指向 `refs/heads/feat/final-fixes`。读取 loose ref；若不存在，读 `packed-refs`。用户给定分支起点为 `c7fffe4`；不一致时停止代码改动，请用户确认，不能自己切分支。

逐一阅读：

- `src/tushenghao/docs/ref/1.md`
- `src/tushenghao/docs/ref/2.md`
- `src/tushenghao/docs/ref/3-2.md`
- `src/tushenghao/docs/ref/4.md`
- `src/tushenghao/docs/ref/5.md`
- `src/tushenghao/docs/ref/v0.8.1_Detector.md`
- `src/tushenghao/docs/block5_acceptance.md`
- 用户提供的全系统审查报告 R01–R09。

创建修改前清单：待改文件路径、SHA256、公共头 SHA256、ref 文件 SHA256、生产配置和模型 SHA256。记录工作区已存在的 build 目录，**不先清空它们**。将清单存入本轮证据目录。

固定目录，以下命令都从仓库根目录执行：

```text
build/final-fixes-release/           本轮 Release 构建及运行临时文件
build/final-fixes-debug/             本轮 Debug 构建及运行临时文件
build/final-fixes-clean/             新用户路径复现的临时源、构建、输入
src/tushenghao/docs/evidence/final-fixes/  本轮永久证据
src/tushenghao/docs/final-fixes_acceptance.md  本轮验收报告
```

三个固定 build 目录若已有内容且无法证明是本轮创建的，不删除、不覆盖；先报告冲突，让用户处理。属于本轮的目录创建 `.final-fixes-owned` 标记，记下创建时的绝对路径。

记录输入视频绝对路径，后续命令用变量 `VIDEO`，不得写入个人目录作为默认值。核对原视频：

| 项目 | 基线证据 |
|---|---|
| SHA256 | `aa1219a7a7b702ea1265be8752853a267c982a651afa0f7846f516f9517c9ac7` |
| 尺寸 / 完整帧数 | 1440×1080 / 1676 |
| 完整结果指纹 | `c1785fb03a54ed25` |
| 状态计数 | DETECTED 864；NOT_DETECTED 812 |

输入 hash 不符时不能套用这组验收数值。报告输入差异，等用户给正确视频。

## 2. 文件级修改地图

所有路径以仓库根目录为起点。新增文件按本表落位，不能临时另建目录结构。

| 文件完整路径 | 类型 | 内容 / 对应项目 |
|---|---|---|
| `src/tushenghao/app/main.cpp` | 修改 | CLI 两个无值开关和 expected-frames 参数；严格冲突检查；中文错误摘要入口 |
| `src/tushenghao/app/offline_runner.cpp` | 修改 | R01 完整性；R05 export 调用；显示/视频/中文摘要编排；R09 拆分 |
| `src/tushenghao/app/offline_runner.hpp` | 保持接口 | `runOffline` 签名不变；仅补必要注释 |
| `src/tushenghao/app/input_completion.hpp/.cpp` | 新增 | 完整输入判定、显式子集判定；R01 独立可测 helper |
| `src/tushenghao/app/display_session.hpp/.cpp` | 新增 | 显示可用性检查、显示生命周期、退出键；不参与检测 |
| `src/tushenghao/app/display_probe.cpp` | 新增 | 单独进程执行 OpenCV GUI 启动探测；允许失败而不杀主进程 |
| `src/tushenghao/app/video_exporter.hpp/.cpp` | 新增 | MJPG/MP4 打开、逐帧写、关闭、读回验证和最终命名 |
| `src/tushenghao/app/run_console.hpp/.cpp` | 新增 | 原英文行 + 中文摘要；状态/原因中文标签表；瓶颈选择 |
| `src/tushenghao/app/debug_renderer.cpp` | 必要时修改 | 复用当前帧 overlay；只整理函数，不改画框有效性规则 |
| `src/tushenghao/lib/config/observability_config.hpp` | 修改 | App 输入完整性、视频编码和 GUI 探测配置；不属于公共 Detector 布局 |
| `src/tushenghao/lib/config/config.cpp` | 修改 | 新配置的加载、允许字段、校验、effective-config 序列化；R03/R05/D |
| `src/tushenghao/lib/pipeline/decode_stage.cpp` | 修改 | R06：分离 geometry 阶段与后续 corner 预算门控；不改判据 |
| `src/tushenghao/lib/pipeline/diagnostics_recorder.hpp/.cpp` | 修改 | R05：export 完成状态补记，DISABLED/null、首次快照、侧表和总计 |
| `src/tushenghao/lib/pipeline/run_report.hpp/.cpp` | 必要时修改 | 内部统计支持；不改公共结果，不改既有指纹算法 |
| `src/tushenghao/tools/validation/observability_verify.cpp` | 修改 | R01/R02/R05：完整性、空比较、expected-frames、export 侧表核验；R09 |
| `src/tushenghao/tools/validation/final_fixes_verify.cpp` | 新增 | 本轮 CLI、视频、路径、链接检查的验证入口；不参与检测 |
| `src/tushenghao/tests/geometry_matcher_test.cpp` | 修改 | 正确模型 fixture；主动断言，保留原测试目标 |
| `src/tushenghao/tests/diagnostics_record_test.cpp` | 修改 | export 开/关和首次快照断言 |
| `src/tushenghao/tests/timing_test.cpp` | 必要时修改 | disabled 不制造测量值；不改原断言预期来迁就实现 |
| `src/tushenghao/tests/observability_integration_test.cpp` | 修改 | 输入完整性、geometry 预算隔离、App helper 集成断言 |
| `src/tushenghao/tests/debug_renderer_test.cpp` | 必要时修改 | 无当前检测不画旧几何；新增功能不破坏原图层规则 |
| `src/tushenghao/tests/config_contract_test.cpp`、`config_error_test.cpp`、`config_roundtrip_test.cpp` | 必要时修改 | 新 App 参数合法性与往返；旧“视频导出未实现”预期改成真实合法性检查 |
| `src/tushenghao/config/detector.yaml` | 修改 App 字段 | 新 App 字段默认关闭/自动；算法及预算原值不变 |
| `src/tushenghao/config/detector_debug.yaml` | 新增 | 可迁移 debug 配置 |
| `src/tushenghao/config/detector_verification.yaml` | 新增 | 可迁移全帧 evidence 配置 |
| `src/tushenghao/CMakeLists.txt` | 修改 | App helper、GUI probe、验证工具构建；CTest 仍保留 23 个目标 |
| `src/tushenghao/README.md` | 修改 | R03/R04/R08；完整命令、输入输出、显示/导出说明 |
| `src/tushenghao/docs/INDEX.md` | 修改 | 当前状态和本轮文档/证据导航 |
| `src/tushenghao/docs/block4_acceptance.md` | 修改 | R07：43 个本地证据链接 |
| `src/tushenghao/docs/block3.md` | 必要时修改 | R08：纠正已过时的“缺 Block4 导致 NOT_READY”；保留未完成的人工标注事项 |
| `src/tushenghao/include/mark/detector_types.hpp` | 仅注释 | 若旧注释仍称 diagnostics 未实现，仅纠正注释；定义和布局不变 |
| `src/tushenghao/docs/diagnostics_schema.md` | 修改 | export 侧表补充字段、完整性字段、旧证据兼容说明 |
| `src/tushenghao/docs/final-fixes_acceptance.md` | 新增，必交 | 13 项验收矩阵、结果、限制和复现方法 |
| `src/tushenghao/docs/evidence/final-fixes/INDEX.md` | 新增，必交 | 证据导航、命令、hash、人工关卡状态 |

禁止修改生产算法文件：`lib/geometry/geometry_matcher.cpp`、`geometry_validation.cpp`、`geometry_l_topology.cpp`、`geometry_observation.cpp`、`geometry_assignment_completion.cpp`；`lib/corners/` 的生产实现；`config/marker_geometry.yaml`；时序平滑和关联算法。发现必须碰这些文件才能完成某项时停止该项，给具体冲突，不自行扩范围。

## 3. A 批：输入、验证、配置、文档和 fixture

### A1. R01：提前 EOF 必须标不完整

基线位置：`app/offline_runner.cpp:39,:55`；`observability_verify.cpp:53,:62`。

原问题：`capture.read()==false` 无条件当正常结束；验证器只核对“已处理的数据内部一致”，没有核对输入是否读完。

新增内部 helper，接口如下；实现细节可以拆函数，但不可改变这里的语义：

```cpp
struct InputCompletion {
    bool incomplete{false};
    bool coverage_verified{false};
    std::uint64_t frames_read{0};
    std::optional<std::uint64_t> expected_frames;
    std::string reason; // 技术原因码，中文在 App 输出层映射
};

InputCompletion evaluate_input_completion(
    std::uint64_t frames_read,
    std::optional<std::uint64_t> expected_frames,
    bool user_stopped,
    bool explicit_subset,
    const std::set<std::uint64_t>& pending_subset);
```

判定顺序：

1. 打开视频后读取 `CAP_PROP_FRAME_COUNT`。正、有限、可表示为 uint64 的整数才接受，不能把 NaN/负数/0 转成有效期待值。保留原始 metadata。
2. 可选 `output.expected_frame_count` 默认为 0（自动）；CLI `--expected-frames N` 可覆盖，N 必须为正整数。显式值优先，manifest 记录来源 `explicit`，不能篡改容器原始值。
3. 全输入运行：有效期待值存在时，读到的帧数必须等于期待值；少读或多读均为 incomplete。无有效期待值且无显式值时，记录 `coverage_unverified`，不宣称完整成功；非零返回，提示提供 expected-frames。
4. 显式 subset audit：只要求请求帧全部存在，记录 `explicit_subset`，不能冒称全视频完整。用户主动提前退出仍为 incomplete。
5. `frames_read` 是实际成功读取数；`submitted` 是实际送入 pipeline 数；subset 下两者不同。不要混用帧 ID、读帧数、提交数。
6. 全输入提前停止时先保留已处理记录，再写 `summary.incomplete=true`、manifest 完整性字段及 `INCOMPLETE.json`（原因、期待数、读取数）；退出非零。不得写 `PASS`，不得因抛异常而丢掉 41 帧诊断。
7. 零帧、打开失败、真实 I/O 异常沿用失败生命周期和 `FAILED.json`；不伪造有效 summary。GUI q/ESC 与输入损坏使用不同原因码。

manifest 的新增完整性数据放 `environment` 下，避免重定义公共接口：`expected_frames`、`expected_frames_source`、`frames_read`、`coverage`、`coverage_verified`、`completion_reason`。另加 `final_fixes_contract: "1"`，用于识别本轮补充校验。未知用明确字符串，不用 0 假装已知。

验证器 `--check-run` 必须拒绝 `FAILED.json`、`INCOMPLETE.json`、summary incomplete，以及全输入且 coverage 未证实的本轮记录。全输入还要核对期待数、读取数、submitted 一致。旧历史 manifest 无新增字段时：使用原有正整数 frame_count_metadata 和 frames_read 核对；不能默认通过。实验生成记录及显式 subset 按其真实范围核对，不要求容器帧数。

**边界说明**：容器 metadata 在任意视频上未必可靠。本轮覆盖的是已核对的 AVI；显式 expected-frames 是额外的完整性证据入口，不是检测阈值。不得声称这能识别所有坏视频。

### A2. R02：空比较拒绝；全视频比较有范围证明

保留现有 `--compare`、`--route-b` 和原公共结果比较规则。追加工具参数 `--expected-frames N`，只用于 compare/check-run；不改变历史数据。

`compare()` 具体改法：

- 两侧非空；任一侧 0 行，报告 FAIL、非零退出。两侧为空也不能 PASS。
- 逐侧帧 ID 严格递增；重复、倒序、两侧 ID/时间戳不同按现有失败机制拒绝。不能先排序、去重再比。
- 不带 expected-frames：仍允许合法显式子集，报告必须注明 `coverage=compared_records_only`、真实比较数；不称全视频回归。
- 带 expected-frames N：两侧必须恰好 N 行，帧 ID 必须依次为 0…N−1；否则 FAIL。该模式用于本轮 1676 帧回归。
- 保留原有排除项；不能顺便排除 corners、orientation、tracks、display 或错误原因。诊断新增的纯观测事件沿用已批准排除规则，不扩大白名单。
- `--route-b` 不能用于本轮最终“零回归”验收；该补丁已在基线中完成。

必测：空/空、空/非空、两侧相同但仅 41 行配 expected1676、重复 ID、倒序 ID、合法显式子集、合法完整比较。

### A3. R03/R04：可迁移配置和新人命令

新建两个配置，分别以已归档 debug/verification 配置为行为基准复制到 `config/`，只修路径、加入本轮 App 字段；保留原算法参数和 diagnostic 图层选择。

`marker_geometry_path: "marker_geometry.yaml"` 按 YAML 所在目录解析，不按 CWD，不依赖 `/home/tushenghao/`。README 指向新配置；历史 evidence YAML 不动。

README 加 Ubuntu 24.04 已验证依赖和版本证据：GCC 13.3、CMake 3.28.3、OpenCV 4.6、OpenSSL 3.0.13 是本轮验证环境，不假称所有版本均通过。项目 C++17、CMake 最低版本以 CMakeLists 为准。

```bash
sudo apt-get update
sudo apt-get install --no-install-recommends build-essential cmake libopencv-dev libssl-dev
# 仅使用需要该头文件的独立工具时安装，不是当前 Detector 必装依赖：
sudo apt-get install --no-install-recommends nlohmann-json3-dev
```

可选验证视频编码使用 `ffprobe`，对应 Ubuntu 包 `ffmpeg`；它不是 App 运行依赖。README 必须把可选验证工具与必装依赖分开。

三个模式分别是：baseline、debug、debug+verification。说明它们的配置、输入位置、run-dir 必须未存在、输出文件、失败退出码、无屏行为。不把 evidence 目录当当前配置目录。

### A4. R07/R08：活文档修复，历史证据保留

- `docs/block4_acceptance.md` 中 43 个旧 build 本地链接改为从该文档出发的 `evidence/block4/...`，逐个确认目标存在。只改链接，不改历史验收数据。
- README/INDEX 写明当前 Block3/4/5 已合入，基线 Block5 Release23/23，Debug 旧 fixture 的失败由本轮修复；最终结果未跑出来前不能预填 Debug23/23。
- 纠正 `docs/block3.md` 中仅因“Block4 未接入”留下的旧状态；保留人工 V/Q 标注等真实遗留项。
- 纠正公共头中 diagnostics 未实现的旧注释，仅改注释。
- 文档标清“基线历史状态”和“本轮验收状态”，不把本轮尚未实施的功能写成已完成。

### A5. fixture：按实际模型构造三个完整 L

`tests/geometry_matcher_test.cpp` 的旧 fixture 有三个问题：三锚点共线；模型 L0/L1/L2 与当前三个完整 L 身份不符；手造观察缺少当前观察层的拓扑证据。

明确替换方案：复用 `test_support/block3_fixture.hpp` 的 `fixture::scene()` 和 `fixture::model()`；**不直接把 fixture 的已知 assignment 作为 matcher 答案输入**。

执行步骤：

1. 载入生产配置和实际 marker_geometry，生成默认非退化 scene。
2. 在测试侧利用 scene 已知的组件对应，仅筛出实际模型 `L0`、`L2`、`L3` 的三个 WhiteComponent，保留非连续 component ID；不得把答案注入观察或匹配函数。
3. 调用真实 `observeShapes(components, geometry_config)`，再调用真实 `generateGeometryHypotheses(observations, model, geometry_config)`。
4. 正例断言：存在至少一个有效三 L 假设；每个被接受假设的三 L assignment 不重复，所用组件确实存在，矩阵为有限的 2×3 仿射矩阵；至少一个候选与测试真值对应相符。允许模型本身存在的合法歧义，不要求候选恰好一个。
5. 保留资源截断字段检查。增加退化负例：三个合法 L 组件平移到共线锚点位置，经真实观察后不得产生可接受的非退化三 L 仿射解释。
6. 用现有 `temporal_fixture::check` 等主动检查替代会被 NDEBUG 删除的 assert。保留原断言意图，Release 和 Debug 都实际执行。

如果默认 scene 的真实观察不满足这一正例，先检查 fixture 筛选、坐标和拓扑构造；不得改 matcher、参数、模型让测试过。出现无法解释的生产逻辑问题时停止并报告。

## 4. B 批：export 计时与 scope 隔离

### B1. R05：一个真实 export 完成状态，只入账一次

原问题：runner 不管 timing 开关都调用 `addExportTiming`；recorder 强制产生 MEASURED；首次快照在写盘前拍下，export 完成后未更新。

保留 existing `addExportTiming` 作为兼容调用，新增内部入口：

```cpp
void complete_export(std::uint64_t frame_id,
                     const StageTiming& completed,
                     bool selected_record);
```

`StageTiming` 是 `lib/core/diagnostics_types.hpp` 中的现有类型。completed.stage 必须为 Export；只接受 MEASURED+非负 elapsed，或 DISABLED+null。错误组合、重复完成、非当前提交帧 ID 拒绝。

执行边界明确：

| 本帧发生了什么 | Export 完成状态 |
|---|---|
| 写 selected JSONL / evidence 图，或写输出视频帧 | timing 开：MEASURED；关：DISABLED/null |
| 没有任何逐帧 export | 保留 NOT_EXECUTED/null，不调用 complete_export |
| 仅结束时写 manifest/summary/关闭编码器 | run_export 成本，不补到最后一帧 |

逐帧 Export 从实际写盘开始，覆盖本帧 JSONL、evidence 图片、视频 write；不含 process、render、wait、补写侧表本身。timing 关闭时不计算或存储 Export 的 elapsed。框架已有独立 run wall/run_export 成本口径保留，文档明确它不是被关闭的阶段采样值。

`submit(record)` 保持先提交原记录，再完成 Export：本帧 JSONL 内 Export 尚是 NOT_EXECUTED，避免自引用。完成后更新 summary 的 Export 状态分母、有效测量列表，并同步 `summary.first_frame` 对应槽。不要修改其他阶段首次值。

侧表 `export_timings.jsonl` 保留旧字段；新增 `status`、`selected_record`，关闭计时时 `elapsed_us:null`。视频可使未采样帧发生 Export：允许其有侧表记录，不强行把它加入 frames.jsonl。

验证器兼容历史侧表（缺 status 但有非负数 elapsed，按旧 MEASURED 解释）；本轮记录严格检查新状态/值组合。取消“侧表 ID 必须属于详细帧 ID”的单一假设：未采样视频帧必须在本轮合法处理范围内；selected_record=true 必须有对应详细记录。核对 Export 总状态数仍等于 submitted，侧表去重，选中记录有且只有一次完成；本轮 summary.first_frame 与首个实际提交帧的侧表完成值相符。历史首次快照缺 Export 不据此追溯修改或拒绝旧证据。本轮 manifest 记录 first_submitted_frame_id 和是否请求 video export，使核验不依赖“第一帧总是 ID=0”的假设。

**不得**把“未测量”记成 0us，或因为视频写出就绕过 timing_enabled。保留 recorder 生命周期、写盘失败标记和重复调用检查。

### B2. R06：geometry scope 不读取后续角预算

基线 `lib/pipeline/decode_stage.cpp:83,:112`。

调整 `runDecodePipeline` 内门控顺序：

```text
示意，非实现：
检查当前 scope 真正需要的 geometry / assignment 前置配置
执行原有 preprocess → observation → hypotheses → validation → assignment
若 scope == Geometry：结束 geometry 记录并返回，不进入 corners / decode / stabilize
否则：检查已有 corner observation budget；缺失仍 NOT_READY
继续原有角点解析与语义流程
```

geometry 所需 assignment 预算不得删除。只把 corner 预算检查移动到其实际消费者之前；缺角预算的 public full/decode 流程仍 NOT_READY。**不能为了 audit 正常把 public Detector 的生产门控解除**。

测试使用同一输入、同一 geometry/assignment 参数，唯一变量是缺失 corner observation budget：Geometry scope 可完成自身流程且 decode/stabilize/public total 为 NOT_EXECUTED；Decode/Full 为 NOT_READY；补齐原生产预算后恢复原输出。

## 5. C 批：R09 独立整理

范围限于本轮需要修改的 App、recorder、验证器及其本轮 helper。不要整库重新命名。

1. 先归档 B 批结束的 23 个测试结果、固定短序列输出和命令输出；短序列至少包含当前检测、失检、恢复、关闭计时及显式 subset，用于拆分前后比较。
2. 格式化长行：代码目标列宽 100–120；不可拆的 URL/字面量可超过，逐条说明。禁止保留 1509 字符的逻辑行。
3. 按职责拆出读取/完整性、单帧处理编排、显示、导出、终端报告、verify 比较/范围检查函数。公共 `runOffline` 等既有签名不变。
4. 新类 PascalCase；新增或被本轮提取的内部函数 snake_case；新 private 成员尾缀 `_`。冻结/批准接口的 camelCase 保留，列入命名例外，不能强行重命名破坏调用。
5. 一般函数超过 80 行、嵌套超过三层时按职责拆分；确实需要保留的串行生命周期说明原因。不是把一行函数简单换行凑行数。
6. 不顺便换容器、缓存、线程、数学式、计时边界或算法参数。
7. 重跑测试与相同输入比较。C 前后公共结果、原因、计时状态、完整性和失败退出码必须一致；只允许耗时数值自然变化、code hash 和源码行号变化。

交付记录单独列“R09 整理前→整理后”，让用户能独立 review。Codex 不创建 commit。

## 6. D 批：显示、导出、终端摘要

### D1. CLI 和 YAML

`--display`、`--export-video` 是无值开关，仅 `marker_app --mode debug` 可用；重复开关、缺值参数、未知参数按原严格 CLI 规则拒绝。baseline + 任一输出开关在打开输入/创建运行目录前报错。

CLI 不改变算法参数；分别设置现有 `output.show_window`、`output.export_video`。新增配置只属于 App：

| YAML 字段 | 默认 | 校验 / 含义 |
|---|---|---|
| `output.expected_frame_count` | 0 | 非负整数，0 自动；仅输入完整性 |
| `output.video_fourcc` | `MJPG` | 本期只接受 MJPG，不暗中换编码 |
| `output.video_filename` | `overlay.mp4` | 本期只接受该文件名，防止路径逃逸；输出归 run-dir |
| `output.display_probe_timeout_ms` | 3000 | 正整数；GUI 探测工程超时，不是几何预算或算法门限 |

同步允许字段、加载、校验、effective config 写出、配置 roundtrip 测试。删除 export_video 的全局 NOT_IMPLEMENTED 拒绝，替换为真实模式/配置合法性检查。baseline 显式拒绝 export_video。

`--expected-frames N` 只影响本次 App 完整性预期。验证器的同名参数影响比较范围，不作用于检测。不得混淆两者。

### D2. --display：无屏降级，有屏使用原 overlay

内部接口：

```cpp
class DisplaySession {
public:
    // probe_path 指向与 marker_app 同构建目录的 marker_display_probe。
    bool start(bool requested, const std::filesystem::path& probe_path,
               std::chrono::milliseconds timeout);
    bool show_and_poll(const cv::Mat& overlay); // true：用户请求停止
    bool active() const;
    const std::string& unavailable_reason() const;
};
```

Linux GUI 预检顺序：

1. 未请求显示：不调用任何 highgui，不创建窗口。
2. DISPLAY、WAYLAND_DISPLAY 都未设置或为空：active=false，打印中文降级信息，继续处理和导出。
3. 设置了环境变量：使用 POSIX 进程 API 启动独立 `marker_display_probe`，不使用拼接 shell 命令。probe 自己调用 OpenCV namedWindow/一次 imshow/waitKey/destroyWindow，成功返回 0。
4. probe 缺失、非零/被信号杀死或超时：active=false，继续无窗运行。超时回收自己启动的子进程；不能杀其他进程。这样 Qt 在无效 DISPLAY 上 abort 只影响 probe。
5. probe 成功后主进程打开窗口，沿用当前 overlay；普通 OpenCV 异常关闭显示并降级，处理继续。记录 requested/active/reason。**不能宣称此预检能保证显示服务在运行中被强制移除时仍绝对不崩**；该极端情况不是本轮保证项。

probe 的定位：从当前可执行文件真实位置取得 sibling 路径，不依赖 CWD/用户家目录；CMake 为 marker_app 和所有可使用 runOffline 的 audit 目标添加 probe 构建依赖。probe 是 App 辅助可执行程序，不是新的 lib 模块。

只在 active 时调用 imshow/waitKey。GUI 成本计 Visualize/Wait，绝不进入 ProcessTotal。q/ESC：正常释放资源，记录 USER_STOP/incomplete，非零退出；不能当完整回归 PASS。

保持 renderer 规则：当前 NOT_DETECTED/empty 不画历史四点；历史显示最多文本层；不能为方便目视把旧 track 当当前 Detection。

### D3. --export-video：MJPG 编码、MP4 容器

重要技术结论：在已验证 Ubuntu/OpenCV 4.6/FFmpeg 环境，`VideoWriter(MJPG, *.mp4)` 可以写出并读回；FFmpeg 将 **容器 tag** 转为 mp4v，但编码仍是 **mjpeg**。本方案探测样本写入/读回 8 帧，ffprobe 报告 `codec_name=mjpeg`、`codec_tag_string=mp4v`。不能把 tag=mp4v 误判成算法选择了 MPEG-4 编码，也不能只看扩展名证明编码正确。[OpenCV VideoWriter 官方说明](https://docs.opencv.org/4.13.0/dd/d9e/classcv_1_1VideoWriter.html)说明 MP4 后端可能转换 fourcc 标签。

内部接口：

```cpp
class VideoExporter {
public:
    void open(const std::filesystem::path& run_dir, cv::Size original_size,
              double source_fps, const std::string& fourcc);
    void write(const cv::Mat& overlay);
    // 必须 release 后读回；完整且核验成功才能改名为 overlay.mp4。
    void finish(bool input_complete);
    std::uint64_t written_frames() const;
};
```

固定逻辑：

- 请求 debug export 后以 `cv::CAP_FFMPEG`、fourcc MJPG 打开 `<run-dir>/overlay.partial.mp4`；宽高为原图，fps 为原视频 FPS，不是处理吞吐率，也不是 playback_fps。
- open/isOpened 失败必须使运行失败，写明编码器/容器不支持。**不自动改为 AVI、mp4v、H264**。换编码需要用户另外批准。
- 每个实际处理帧写一张 overlay，按源帧顺序，独立于 detail sampling。无检测帧也写当前帧背景与合法文本，不补旧几何。
- write 前检查 CV_8UC3、原图大小、非空；异常失败不得留下成功文件。`VideoWriter::write` 无成功布尔值，不能据此假称写盘已成功。
- 关闭时 release；重新用 VideoCapture 逐帧解码，核对帧数、宽高和 FPS（允许容器对 FPS 的表示舍入，使用原帧周期是否一致检查，不自设宽松检测门限）。记录 requested_fourcc/backend/readback 数据。
- 输入完整且读回等于实际写帧数，才把 partial 改名 overlay.mp4。损坏输入或 q/ESC 保留明确命名的 partial，不冒充完整交付。
- write 成本在逐帧 Export；open/release/readback/rename 是运行级导出成本，在 run_export 补充报告列出。不可塞入 process 或最后一帧。
- mp4 写出成功不等于处理达到视频帧率；README 明确播放帧率与处理耗时的区别。

结束顺序必须为：判定输入完整性 → 关闭并读回视频（仍处 recorder Running 生命周期）→ 更新运行级导出成本 → 写完整性标记/最终 manifest → finishRun 写 summary → 写 run_export 补充。编码失败必须在 finishRun 之前触发失败标记，不能先写成功 summary 再发现输出视频坏了。输入不完整但 partial 读回成功时仍写不完整 summary，不改成成功。

MJPG 是有损编码，overlay 不用于反推原始角点误差；原始定位证据继续引用原图/已有 evidence。

### D4. 中文终端摘要

保留原英文行格式与字段：

```text
run=... frames=... fingerprint=... incomplete=...
```

紧接其后追加中文，不替换原行：

```text
运行结果：完整 / 不完整 / 失败
帧数：读取 N，处理 M；检测 K，未检测 L
耗时：处理平均 X ms，p95 Y ms；运行总耗时 Z s
瓶颈阶段（已测样本平均值）：语义解码 X ms
```

“瓶颈”只从实际有 MEASURED 样本的 Preprocess/Detect/Decode/Stabilize 四个算法阶段按平均耗时选最大项。相同值按现有 Stage 顺序决定；注明是测得样本平均值。不能拿 Capture/Wait/Export 当算法瓶颈，也不能拿 public total 和子阶段重复比较。

计时关闭：显示“阶段计时未启用；瓶颈阶段不可判定”，不输出假 0ms。scope 未执行完整 pipeline：注明范围，只比较实际执行阶段。0 帧失败不计算均值或 p95。

零帧/打开失败没有有效 final summary 时，只追加中文失败说明；不得为满足英文行检查而造一条成功 run 行。正常及有已处理帧的完整/不完整结束，保留原英文行。

对已有 status/reason code 建明确中文标签映射；未知原因显示“未映射原因”并原样保留技术码。指纹、路径、run ID、技术原因码不改写；数字用阿拉伯数字配中文单位。原异常详情保留，另加中文类别说明，不把未知错误猜成阈值/视频问题。

## 7. 构建与验证命令

以下是执行命令模板，不是本方案已跑过的修复结果。`VIDEO` 指向已核对原视频；所有 run-dir 和 report 文件必须是新路径。重复验证用新的 run 名，不能覆盖旧证据。

### 7.1 两种构建和原 23 个测试

```bash
cmake -S src/tushenghao -B build/final-fixes-release \
  -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=c7fffe4-final-fixes-working-tree
cmake --build build/final-fixes-release -j4
ctest --test-dir build/final-fixes-release --output-on-failure

cmake -S src/tushenghao -B build/final-fixes-debug \
  -DCMAKE_BUILD_TYPE=Debug -DMARK_COMMIT_LABEL=c7fffe4-final-fixes-working-tree
cmake --build build/final-fixes-debug -j4
ctest --test-dir build/final-fixes-debug --output-on-failure
```

CTest 保留 23 个注册目标。新增断言放入对应现有测试；专用验收工具不注册成第 24 个 CTest 来改变用户约定分母。标签不是当前准确 commit，manifest 中继续保存真实源码 hash。

### 7.2 README 三模式与全视频零回归

```bash
VIDEO=/absolute/path/to/marker_video.avi

build/final-fixes-release/marker_app --check-config \
  --config src/tushenghao/config/detector.yaml

build/final-fixes-release/marker_app --video "$VIDEO" \
  --config src/tushenghao/config/detector.yaml --mode baseline \
  --run-dir build/final-fixes-release/runs/baseline

build/final-fixes-release/marker_app --video "$VIDEO" \
  --config src/tushenghao/config/detector_debug.yaml --mode debug \
  --run-dir build/final-fixes-release/runs/debug

build/final-fixes-release/marker_app --video "$VIDEO" \
  --config src/tushenghao/config/detector_verification.yaml --mode debug \
  --run-purpose verification --export-video \
  --run-dir build/final-fixes-release/runs/verification

build/final-fixes-release/observability_verify --check-run \
  build/final-fixes-release/runs/verification --expected-frames 1676 \
  --report build/final-fixes-release/check-full.json
```

三模式都必须处理完整 1676 帧并通过完整性核验。verification 配置全帧 detail_interval=1，不用采样文件冒充全视频比较。

比较基线使用 `src/tushenghao/docs/evidence/block5/step0/patched-baseline.jsonl`，同目录 `patched_baseline_manifest.yaml` 记录1676帧、864/812；先核对真实行数、0…1675 ID 范围、指纹，以及该 JSONL 的 SHA256：`0f2a0cb8c80146124a18d6e7ff72a290d225a1266f7b5a36fb3249c3cc49e235`。不能选 original-baseline/route-b 之前的结果，也不能选空/采样文件。若实际分支缺该金样，则在修改前用 c7fffe4 工作树构建生成一次基线并存证；不可由修改后的代码倒造基线。

```bash
# GOLDEN 必须通过前述基线检查，变量记录到 commands.json。
build/final-fixes-release/observability_verify --compare "$GOLDEN" \
  build/final-fixes-release/runs/verification --expected-frames 1676 \
  --report build/final-fixes-release/regression-full.json
```

必须比较公共结果及原有语义原因，不能仅比计数/指纹就省掉结构比较。预期仍是 864/812、指纹 c1785fb03a54ed25、零失败帧。不使用 route-b 宽容模式。

Debug 至少跑全部 23 测试、配置三模式启动和损坏输入验证。最终 Release 的 verification 全视频同时承担逐帧回归、Export 侧表、视频导出和中文输出验收；Debug 全视频一次承担另一构建的回归。不为视频输出单独重复运行全视频。baseline 全视频只测一次现有性能；普通 debug 可以用10帧短 fixture做模式验证。阶段 C 用固定短序列前后对账，最后全视频比较作为总防线。

### 7.3 损坏输入与空比较

```bash
mkdir -p build/final-fixes-release/fixtures
head -c 262144 "$VIDEO" > build/final-fixes-release/fixtures/truncated-41.avi

build/final-fixes-release/marker_app \
  --video build/final-fixes-release/fixtures/truncated-41.avi \
  --config src/tushenghao/config/detector_verification.yaml --mode debug \
  --run-dir build/final-fixes-release/runs/truncated-41
# 预期非零；实际退出码单独记入证据，不用“|| true”掩盖。

build/final-fixes-release/observability_verify --check-run \
  build/final-fixes-release/runs/truncated-41 \
  --report build/final-fixes-release/check-truncated.json
# 同样预期非零。

touch build/final-fixes-release/fixtures/empty-a.jsonl
touch build/final-fixes-release/fixtures/empty-b.jsonl
build/final-fixes-release/observability_verify --compare \
  build/final-fixes-release/fixtures/empty-a.jsonl \
  build/final-fixes-release/fixtures/empty-b.jsonl \
  --report build/final-fixes-release/compare-empty.json
# 预期 FAIL，退出非零，不能 PASS frames=0。
```

截断样本在目标环境预期解出 41 帧；如果解码器版本导致实际数量不同，记录真实数，仍必须提前停止且 incomplete。不得把“必须恰好 41”当生产判断算法。

### 7.4 无屏、显示与视频

```bash
env -u DISPLAY -u WAYLAND_DISPLAY build/final-fixes-release/marker_app \
  --video "$VIDEO" --config src/tushenghao/config/detector_debug.yaml \
  --mode debug --display --run-dir build/final-fixes-release/runs/headless-display

# 完整视频导出复用7.2的verification运行，无需再跑一次。

# 若已安装可选 ffmpeg 验证工具：
ffprobe -v error -select_streams v:0 \
  -show_entries stream=codec_name,codec_tag_string,width,height,nb_frames,avg_frame_rate \
  -of json build/final-fixes-release/runs/verification/overlay.mp4
```

无屏运行完整且退出 0，有中文降级说明、不创建窗口；不存在 display 服务的非空 DISPLAY 必须也由 probe 隔离失败，主进程继续。单测用短合成输入完成这两项，避免多次完整视频运行。

有屏人工命令与无屏命令相同，去掉 env 前缀。用户确认窗口出现、画面前进、q/ESC 后 incomplete；Codex 没有可用屏幕时标“待人工验收”，不得填“通过”。

输出视频必须读回 1676 帧、1440×1080；可用 ffprobe 时 codec_name=mjpeg，允许 container tag=mp4v。随后用户在自己的播放器打开确认可播放。验证器不能只检查文件存在/大于 0。

### 7.5 本轮独立验证工具约定

新增 `final_fixes_verify` 复用现有 JSON reader、OpenCV、主动 check，不引入框架。支持以下固定子命令；参数缺失/重复/未知均拒绝，报告 JSON 不覆盖已存在文件：

```bash
build/final-fixes-release/final_fixes_verify links \
  --doc src/tushenghao/docs/block4_acceptance.md \
  --report build/final-fixes-release/links.json

build/final-fixes-release/final_fixes_verify video \
  --input build/final-fixes-release/runs/verification/overlay.mp4 \
  --expected-frames 1676 --width 1440 --height 1080 \
  --report build/final-fixes-release/video-readback.json

build/final-fixes-release/final_fixes_verify console \
  --input build/final-fixes-release/logs/baseline.stdout \
  --report build/final-fixes-release/console.json
```

links 检查 Markdown 本地文件引用，忽略网络链接和页内锚点；正确处理 URL 编码、空格、尖括号路径。报告修复的 43 个旧链接及其目标，全部存在。video 逐帧读回并核宽高/帧数；console 核对原英文行和中文帧数/耗时/瓶颈，不把非空 stdout 当通过。

R05 开关组合、R06 scope 隔离等可直接在现有 integration/recorder 测试中验证，不另造会重复检测算法的验收实现。

## 8. 最终验收矩阵

所有下列必达项通过才允许写“Final-fixes 完成”。屏幕/播放器人工项未验证时，状态写“代码与自动验证完成，待人工 review”，不冒充最终完成。

| ID | 操作 | 通过条件 | 失败先查 |
|---|---|---|---|
| F01 / R01 | 截断样本跑 App + check-run | 非零、incomplete=true；41 帧或实际已解帧保留；期待1676；check-run FAIL | InputCompletion、metadata 来源、summary 生命周期 |
| F02 / R02 | 空比较及范围负例 | 空/空拒绝；41 行配1676拒绝；重复/倒序拒绝；合法子集只宣称子集 | compare 读取循环与范围检查 |
| F03 / R03 | 新路径运行三配置 | 不需要 `/home/tushenghao/`；模型相对 YAML 定位；实际模型 hash 相同 | config resolve、README 配置路径 |
| F04 / R04 | 干净 Ubuntu 按 README 操作 | 安装命令无隐含依赖；可构建、check-config、三模式运行 | CMake 依赖及配置/视频说明 |
| F05 / R05 | timing开/关 × selected/未采样视频帧 | 开时真实 MEASURED；关时 DISABLED/null；无export NOT_EXECUTED；首帧补全；分母=submitted；无重复记账 | complete_export、first_frame、side reader |
| F06 / R06 | 缺角预算的 Geometry/Decode/Full | Geometry 独立完成；后两者仍NOT_READY；未执行阶段不造值 | scope 门控位置 |
| F07 / R07 | 链接工具核查 | 43 条修复链接均存在；历史证据内容hash未改 | 相对路径起点、归档原目标 |
| F08 / R08 | README/INDEX/block3 注释核查 | 当前状态一致；历史数据与本轮数据分清；真实遗留项保留 | 旧Block4/19/NOT_READY陈述 |
| F09 / R09 | C批前后对账 + 可读性检查 | 不改公共签名；无超长逻辑行；拆分职责清楚；行为零差异 | 提取helper时状态/生命周期搬移 |
| F10 / fixture | 两种构建执行原 matcher 测试 | 正确三L模型正例；退化负例；Release主动断言；不改生产算法 | fixture coords/topology/筛选 |
| F11 / display | 无屏、无效DISPLAY、有屏 | 无屏不崩且继续；probe失败降级；有屏人工确认；q/ESC incomplete | probe sibling 路径、runtime active 状态 |
| F12 / video | MJPG→MP4，读回+人工播放器 | 完整输出1676帧；尺寸/FPS一致；codec=mjpeg；不完整只留partial | writer backend、close/readback/rename |
| F13 / 中文 | 正常/关计时/损坏/0帧 stdout/stderr | 原英文行保留；中文数量/耗时/瓶颈；无样本不造统计；技术ID不翻 | run_console标签及统计选择 |

共同必达项：

- Release **23/23**，Debug **23/23**，不排除 fixture、不删测试。
- 完整1676帧结构比较零失败，状态864/812、结果指纹保持；模型 hash 不变，算法参数逐项等值。YAML 加 App 字段会改变整个文件 hash，须记录新旧 hash，不能要求整份配置字节相同。
- `docs/ref/`、历史 evidence 和公共类型布局未被改动。公共头仅允许已列出的注释修订。
- baseline 不产生窗口/视频/evidence图，不把GUI、Wait、export算进处理耗时。
- 导出/显示的 detail sampling 不改变检测结果和时序输入。
- 必要文档、证据 hash 清单、执行命令及退出码齐全；清理后证据可读、链接仍有效。

建议目标而非本轮阻塞：减少说明重复、补“先查哪个文件”的故障排查小节；按新测量提出性能专项候选。不得顺手实施性能改动。

## 9. 干净 Linux / 新用户路径复现

必须真实验证，不能以“我现在机器编过”替代。

1. 新建本轮所有权目录下的临时源副本，仅复制源码、配置、现行文档、必需测试资源；不复制现有 build、可执行文件、CMakeCache。
2. 将副本放到与原用户目录无关的路径，输入复制到该临时环境 `data/raw/marker_video.avi`；核对源 SHA256。
3. 使用干净 Ubuntu 24.04 环境，按 README 安装依赖。优先使用现成可用的容器/隔离环境，不给用户宿主安装未授权工具；缺条件时说明阻塞，不把普通目录搬迁冒充干净系统。
4. 在干净环境构建，跑 Release23/23、check-config、README 三模式的启动/合法输出验证。完整视频算法回归可复用同版本原环境结果，但干净环境运行测试须注明输入是完整视频还是短 fixture；不混写。
5. 对配置路径迁移的硬项，三模式至少各跑一个真实短视频至完整结束；另外在新路径加载原视频及模型验证来源。记录输入帧数，不能误写“新机跑完整1676”除非实际跑了。
6. 检查三份当前配置/README 命令无个人绝对路径。归档 manifest 中记录的是实际运行路径，允许绝对路径作为历史事实，不能以“禁绝对路径”删除这些证据。
7. 归档 OS、编译器、OpenCV、OpenSSL、安装命令、构建日志、测试日志、三模式退出码、实际模型hash。缺任何运行条件写未验证，不写通过。

## 10. 证据归档与安全清理

永久目录：`src/tushenghao/docs/evidence/final-fixes/`。

```text
final-fixes/
  INDEX.md
  environment/        环境、依赖、修改前/后源码与配置hash
  batch-a/            输入失败、空比较、fixture、链接、配置证据
  batch-b/            export开关与scope隔离
  batch-c/            重构前后对账、可读性检查
  regression/         全视频结果、指纹、比较报告、Release/Debug测试日志
  app-output/         headless/probe、视频读回、中文stdout、人工确认记录
  clean-linux/        干净系统和路径搬迁证据
  archive/
    commands.json     参数、CWD、退出码、时间；不是只有命令字符串
    artifact-manifest.json  相对路径、size、SHA256
```

保留可核查的完整回归 JSONL、manifest、summary、effective config、export 侧表、失败标记、报告、日志。视频用于人工复核的最终完整 overlay.mp4 也归档；不要只归档视频 hash 而删除唯一视频。拷贝后以归档路径重新运行读取/比较/链接检查。

历史 manifest 的 source/model 路径是运行事实，保留原样。验收工具验证实际输入需要运行文件存在时，应明确传入当前输入或在验收期间完成核验；**不要把归档复制当可任意重放的自包含 run**，也不要为重放修改历史 manifest。

清理铁律：

1. 只删除本轮创建且有 ownership 标记的三个固定 build 目录，以及本轮隔离环境临时资源。
2. 删除前解析绝对路径，确认在本仓库 `build/` 下且名称精确匹配；排除符号链接指向仓库外的情况。
3. 确认每项永久证据已拷贝、hash/size一致、验收工具读得出；验收报告/INDEX不再引用本轮 build 路径。
4. 使用同一个 shell 完成路径验证与删除。不能 PowerShell 枚举后传给 cmd/batch 拼接递归删除命令。
5. 不执行“清空整个 build”；不删除其他 Block 的 build、synthetic_gen 构建、用户源码副本或未确认旧目录。列出现存其他 build，交用户另行处理。
6. 不把本轮编译缓存/对象文件/二进制搬入永久 evidence。验证通过后把归档工具目标所需证据读回完成，再清理本轮二进制。
7. 生成 artifact manifest 时排除该 manifest 自身，避免自引用 hash；记录清理动作与最终目录状态。

本次方案制定时检查的审查副本另有 `build/final-review-release/`、`build/final-review-debug/`。它们属于上轮审查构建，**不是用户当前分支现场已核对的目录**。若执行现场也有同名目录，只有在确认是可重建构建产物、无未归档证据、无现行文档/fixture依赖且所有权明确后，才列入额外清理清单；否则仅报告，不能据同名推定可删。

## 11. 必交文档

`docs/final-fixes_acceptance.md` 中文写，参考此前四段式：

1. **完成内容**：13项矩阵、各文件实际变更、公共接口未变、各批次状态。
2. **验证方法**：真实环境、输入hash、命令索引、Release/Debug23结果、完整比较、负例、显示/视频证据；没有跑的明确写未验证。
3. **踩坑记录**：容器metadata与实际读取、空比较、Export自引用和禁用、MJPG的MP4 tag、GUI探测、配置相对路径。
4. **已知限制**：812空帧待目视、约94ms性能未优化、其他损坏变体未覆盖、GUI服务运行中强制消失不作绝对保证、人工关卡状态。

README 增补三模式命令、两个新开关、expected-frames、输入输出和错误处理，中文终端说明和故障定位。INDEX 登记本轮验收报告和 evidence INDEX；更新 diagnostics_schema 的增补语义与旧证据兼容。允许把必要故障排查放进现有文档，不另散放。

不允许根据预期填入 PASS、23/23、1676、codec 等结果。只有真实测试证据能填这些值。

## 12. Codex 执行提示词（可直接粘贴）

```text
在 feat/final-fixes 实施《Final-fixes：13 项修复——Codex 完整执行方案》。

你只改文件，不执行任何 Git 命令。用户负责 commit/push/merge。

第0步：只读 .git/HEAD/ref 确认分支和 c7fffe4 起点；阅读方案列出的冻结文档；
记录原始源码、公共头、生产配置/模型、ref 和历史 evidence 的hash；
核对原视频SHA256；检查三个固定build目录所有权。
分支/输入/既有文件冲突不明时停下报告，不自动替用户解决。

按 A→B→C→D 实施：
A：截断输入 incomplete+非零；空/短回归拒绝；portable configs；安装命令；
43个Block4链接；当前文档状态；用真实模型/观察层修旧fixture。
B：export timing开关、首帧快照和侧表完整性；geometry scope角预算隔离。
C：单独做格式化及内部helper拆分；不换算法；重构前后行为对账。
D：debug --display / --export-video；无屏降级和独立GUI probe；
MJPG写MP4，读回核验后才发布完整文件；保留原英文行，追加中文统计。

所有实质改动有中文注释，说明旧问题和改法原因。
保持六个lib模块，按文件地图落位。公共签名与Detection/FrameResult/DisplayState不变。
不改docs/ref原文、历史evidence原件、生产算法、预算数值、模型或阈值。
不做性能优化，不自动更换视频编码，不让采样改变时序或检测结果。
不删测试换绿，不把detections>=1当完成，不把无屏自动测试当有屏人工通过。

Release23/23、Debug23/23；全视频1676逐帧零回归；截断/空比较负例；
export开关/scope隔离；新路径三模式；无屏/无效DISPLAY；视频逐帧读回；
中文正常、关闭计时、失败和0帧边界；都按矩阵执行。
复用验证运行，不反复跑同一全视频消耗额度；出现新失败再追加定位运行。

必交 docs/final-fixes_acceptance.md（完成内容/验证方法/踩坑记录/已知限制），
更新 README、docs/INDEX.md、diagnostics_schema.md；中文写。
全部证据归 docs/evidence/final-fixes/，包含命令/退出码/hash清单和导航。
归档核验完成后只清理本轮有标记的固定build目录；不清整个build、不删他人旧目录。

人工屏幕/播放器验证没有条件时，交付“自动验证完成，待人工review”，
明确列出用户命令和预期画面。其他未达必达项保持未完成，不虚报。
最终报告：13项结果、实际改动文件、测试/回归数、未验证项、已知限制、
证据与验收文档路径、清理范围；不执行Git。
```

## 13. 用户 review gate

本轮没有需要另批准的几何预算/阈值，也没有公共 ABI 变更。以下是交付后的验证关卡，不授权 Codex 猜结论：

- **G-DISPLAY**：用户有屏运行确认弹窗、连续显示及退出行为。
- **G-VIDEO**：用户播放器打开完整 MP4 确认可播放；自动读回/编码验证先完成。
- **G-CLEAN**：若 Codex 环境不能建立干净 Linux，则报告硬项未验证，请用户提供环境，不冒称完成。
- **G-SCOPE**：若必须修改被禁止的算法/冻结接口，停止对应项，提交冲突及最小变更建议，等用户批准。

用户据完整证据 review，再自行进行版本管理。812 空帧目视判定和性能专项在本轮之后单独推进。
