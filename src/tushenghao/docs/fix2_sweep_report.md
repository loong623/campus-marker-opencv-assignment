# fix2-sweep：700 帧人工标注与根因分析

## 结论与当前状态

用户按类别给出的人工判断已登记：A、B 共 588 帧“不该检出”，C、D 共 112 帧“该检出”。700 行 index.csv 已按此填写，来源为用户本次整体复核结论，不冒充逐行独立标注。步骤 1–7 已完成；本次仅修改索引、分析证据和报告，正式代码及配置未改，修改方案待用户审核。

**112 帧的主要问题是原图角点取证预算与真实像素轮廓不匹配，涉及两种机制：111 帧可只调整直线平均残差预算恢复；784 帧被模型有限边的位置预算拒绝。** C、D 是历史分组不同，底层失败阶段相同。A、B 的正常拒绝逻辑应保留。

独立工具用原视频逐帧解码、读取已记录的完整候选，并调用未修改的生产角点、证据校验、四边形校验、语义和 float 发布函数：原预算复现全 1676 帧状态、测量数量和 976 个成功帧的原始角点及方向；内存中试用残差 0.75 px、模型边位置 9.5 px 后，C/D 的 112 帧全部通过整帧验证，A/B 的 588 帧仍拒绝，原 976 成功帧无检出丢失。

这是冻结上游候选的离线因果实验，尚不是修改后正式程序的全流程验收。它没有重跑三 L 生成/M/S 补全、时序及显示，也没有新增角点像素真值；不能把上述结果宣称为最终定位精度、正式性能或所有视频上的保证。

## 人工真值与每类数量

| 类别 | 原失败阶段 / 分组 | 帧数 | 有（该检出） | 无（不该检出） | 人工结论 |
| --- | --- | ---: | ---: | ---: | --- |
| A | 无几何假设 | 564 | 0 | 564 | 明显出画面，被边缘切割，不完整；未检出正确 |
| B | M/S 补全入口拒绝父假设 | 24 | 0 | 24 | 位于临界边缘，只有很不明显的切割；未检出正确 |
| C | NO_VALID_ADJACENT_EDGES | 106 | 106 | 0 | 完全在画面内；漏检 |
| D | Path A 历史残留 | 6 | 6 | 0 | 完全在画面内；漏检 |
| 合计 | 互斥，覆盖全部 NOT_DETECTED | 700 | 112 | 588 | |

标注入口：[index.csv](evidence/final-fixes/fix2-sweep/index.csv)。人工来源记录：[manual_label_authorization.json](evidence/final-fixes/fix2-sweep/manual_label_authorization.json)。图片路径相对于索引目录，帧号零基，700 张 1440×1080 PNG 已逐像素核对原视频解码结果，无缩放、裁切或叠加。

原日志没有 `details.geometry`，实际读取 `details.generated/validated/completed.hypotheses`，与 counts 核对；D → A → B → C 的分类优先级避免重复。

## 分析依据与复算可信度

原诊断为 `evidence/final-fixes/path-a/verification-release/frames.jsonl`。选出的 700 行原诊断完整保存在 [not_detected_frames.jsonl](evidence/final-fixes/fix2-sweep/not_detected_frames.jsonl)。本次没有写入 path-a/。

| 复核事项 | 结果 |
| --- | --- |
| 700 行人工结论按用户类别判断登记 | 700/700 |
| 112 个首个失败角点：原图组件来源唯一、未被边界截断 | 112/112 |
| 原预算角点复算诊断与原日志逐字一致 | 112/112 |
| 只采集统计的工具副本与生产 fit 的诊断一致 | 112/112 |
| 原预算整帧状态、测量数量复现 | 1676/1676 |
| 原 976 成功帧 raw 角点和方向与日志一致 | 976/976 |
| 0.75 残差、9.5 位置预算：恢复 C、D | 112/112 |
| 同一试验：A、B 新增误检 | 0/588 |
| 同一试验：原成功帧丢失 | 0/976 |
| 代码、原视频及封存 Path A 文件哈希不变 | 1315 个受保护文件通过 |

完整结果：[analysis_summary.json](evidence/final-fixes/fix2-sweep/analysis_summary.json)。112 帧逐帧归因：[root_cause_by_frame.csv](evidence/final-fixes/fix2-sweep/root_cause_by_frame.csv)。全视频复算：[full_frame_replay.jsonl](evidence/final-fixes/fix2-sweep/full_frame_replay.jsonl)、[candidate_frame_replay.jsonl](evidence/final-fixes/fix2-sweep/candidate_frame_replay.jsonl)。原预算拒绝计数与敏感性试验：[corner_replay.jsonl](evidence/final-fixes/fix2-sweep/corner_replay.jsonl)、[corner_sensitivity.jsonl](evidence/final-fixes/fix2-sweep/corner_sensitivity.jsonl)。

本次运行状态以 [analysis_manifest.json](evidence/final-fixes/fix2-sweep/analysis_manifest.json) 为准；manifest.json、artifact_hashes.json 保留首次导出时的历史快照，人工标注和报告更新后的当前哈希另见 analysis_hashes.json。

## A 类：正常拒绝的来源

人工观察是明显切割或出画面；诊断中 564 帧全部 generated/validated/completed/measurements 为 0。具有合法 L 拓扑支持的组件数分别为：0 个 366 帧、1 个 126 帧、2 个 72 帧，没有一帧具备三 L 搜索所需的三个合法组件。因此在 geometry_matcher 的三 L 组合生成处自然没有候选，随后没有可发布测量。

注意：401 帧的已提取组件均没有 touches_border 标记，这不推翻用户观察。已离开画面的结构不会成为可提取组件；未碰边的可见残片或背景也不能证明目标完整。不能用“当前没有组件碰边”替代完整性判定。

判断：A 类不是本轮应修复的漏检。保持三 L 的实际拓扑来源与完整观测约束，不能为凑齐 700 个检出而补模型角或放宽缺片规则。

## B 类：不是 M/S 匹配预算不足

24 帧均 generated=3、validated=1、completed=0。每个通过验证的三 L 父假设都至少绑定一个 touches_border 的 L 组件，completeness=0，即 CLEARLY_INCOMPLETE；validation_residual=0，父仿射合法。

精确调用链是：geometry_validation.cpp 的 evaluateCompleteness 检出绑定组件碰边 → 标记 CLEARLY_INCOMPLETE → geometry_assignment_completion.cpp 第 40 行以 INVALID_PARENT 拒绝 → expansions/candidates_examined/output_branches 全为 0。

因此这 24 帧根本没有进入 M/S 候选匹配，不能据其分类名称就调整 M/S 的面积、边界、方向或拓扑预算。用户肉眼观察的“极轻微切割”与代码的边界完整性拒绝一致。后续角点修法保持这些门控，离线试验仍然 24/24 未检出。

## C 类：106 个漏检的两种根因

106 帧均已完成六片绑定，未发生搜索截断，所有提取组件均未碰工作图边界；原图取证也找到唯一完整轮廓。105 帧 generated=3、validated=1、completed=1；另一帧 generated=12，但同样只有一个 validated/completed 假设。因此“没有几何假设”“M/S 没找到”“预算搜索被截断”均不是这 106 帧的失败来源。

resolveObservedCorners 按 P0→P1→P2→P3 顺序取证，第一角失败就返回。日志中首个失败角分布为 P0=7、P1=56、P2=33、P3=10。这个分布是首个阻断位置，不是每帧四角所有潜在问题的统计。

### R1：105 帧，平均残差限制先拒绝真实支持弧

corner_edge_fit.cpp 第 29 行同时要求连续支持弧跨度至少 14 px、平均点线残差不超过 0.5 px。原图阈值轮廓保留 CHAIN_APPROX_NONE 的全部像素，栅格边缘存在一像素起伏；“真实直边存在”并不意味着连续像素弧的平均残差一定 ≤0.5。

只把 max_line_fit_error 从 0.5 调到 0.75，C 类除 784 外的 105 帧均通过失败角点；在四角、证据重算、屏幕排序、语义和 float 发布验证后，这 105 帧全部恢复整帧检出。方向 3°、位置 9 px、跨度 14 px、连接 13.5 px、延伸 9.5 px、角点误差 4.131595 px 等其余约束均保持原值。没有删轮廓点、跨弧拼支持或拿模型角作输出。

把 C/D 合并看，111 个恢复角点所选两条边的较大平均残差介于 0.5000001412–0.6037560241 px。部分样本几乎贴着 0.5 的浮点边界，另一些确实超过 0.5 较多；加一个机器精度 epsilon 无法解决全部问题。光学成像、阈值化与栅格化使边界起伏是合理解释；本次实验直接证明的是 0.5 的硬门控导致漏检，而不是区分每帧噪声到底由哪一项成像过程产生。

更深一层的拒绝链：合格长弧被残差门限删掉后，只剩远离拐角的短直弧，可能只剩一种边身份；即使两种边都有候选，支持端点也可能隔得太远，从而被 connection 或 support_extension 拒绝。这解释了为何日志看似“连接失败”，改残差门限却能解决；连接计数大不等于连接预算就是原始根因。

| 原预算在 C 类的直接拒绝机制 | 帧数 |
| --- | ---: |
| 缺少一种指定外边的合格支持弧 | 32 |
| 两种外边均有候选，但连接全被拒绝 | 51 |
| 已进入交点检查，延伸/交点条件拒绝 | 23 |

例如 [4 帧原图](evidence/final-fixes/fix2-sweep/frames/C/frame-000004.png) 的 P2：原预算有 428 条合格弧和 1187 个进入最终检查的边对，但这些边对全部仅被延伸门限拒绝。最接近通过的边对角点误差为 2.6075 px，延伸为 10.4031/1.0251 px，第一条超过 9.5 px。试用 0.75 后，真实连续支持向角点延伸，所选两边平均残差仅 0.50737/0.26359 px，其他门限不动即可通过。直接放宽延伸虽然能救这帧，却不能处理大多数其他帧。

[21 帧原图](evidence/final-fixes/fix2-sweep/frames/C/frame-000021.png) 的 P1 原预算只有一种指定边的合格弧；试用 0.75 后两边平均残差为 0/0.59678 px，恢复四角。这也解释了 M 所在 P1 为何占首个失败角的大头，但不能仅凭这一分布声称 M 的轮廓一定错误。

### R2：784 帧，模型有限边的位置范围过紧

[784 帧原图](evidence/final-fixes/fix2-sweep/frames/C/frame-000784.png) 完整在画面中。其三 L 仿射为 `[1.515625,-0.03125,108.125; 0.03125,1.59375,211]`，M/S 绑定通过，但 P1 原预算只有 65 条属于第一条指定边的弧，没有第二条边的合格弧。把残差放到 0.75 甚至 1.0 都不能恢复。

单独把 max_edge_position_distance_px 从 9 调到 9.25 即通过。恢复边对的平均残差为 0/0.45256 px，方向差为 1.18119°/2.43795°，最大点到指定有限模型边距离为 4.92165/9.00930 px；第二条边超原预算约 0.00930 px。角点误差 1.85752 px、延伸 3.56537/4.96335 px，其他条件均合格。

根因是模型投影位置门限拒绝了真实外边。三 L 锚点生成的仿射在这三个锚点上 residual=0，不代表整个目标所有外边投影都精确；原图像素映射和模型外推仍有位置差。这 0.00930 的超限不能被描述成浮点舍入错误，9.00930 是实际测得的支持距离。

精细敏感性结果见 [position_sensitivity.jsonl](evidence/final-fixes/fix2-sweep/position_sensitivity.jsonl)。9.25、9.5、10 等均取得同一角点；没有必要据此把位置预算直接放到 15。

## D 类：6 帧共享 R1，382 另有竞争保护放大效应

| 帧号 | 首个失败角 / 候选 | 0.75 重试所选两边平均残差 px | 原始失败的后续效应 |
| --- | --- | --- | --- |
| 153 | P2 / 0 | 0.58863 / 0 | 无四角测量 |
| 280 | P3 / 0 | 0.55593 / 0.23127 | 无四角测量 |
| 382 | P2 / 1 | 0.54687 / 0 | 候选 0 已成功，候选 1 失败，整帧竞争拒绝 |
| 648 | P1 / 0 | 0 / 0.51882 | 无四角测量 |
| 1108 | P0 / 0 | 0.50940 / 0 | 无四角测量 |
| 1573 | P1 / 0 | 0 / 0.51869 | 无四角测量 |

6 帧均有两种指定外边的候选，但原预算没有任何边对通过连接检查。只改变残差门限即可恢复全部 6 帧；D 不需要一套独立的新算法，也不是继续扩大 Path A 锚点搜索的问题。

[382 帧原图](evidence/final-fixes/fix2-sweep/frames/D/frame-000382.png) 已有两个完整候选，原测量数为 1；decode_stage.cpp 第 55–56 行因为另一个候选角点失败而记录 UNRESOLVED_COMPETING_GEOMETRY。重试后两个候选均获得四角，通过几何一致性归并，方向仍唯一，最终发布一个 Detection。因此应解决失败候选的取证问题，不能简单忽略它以强行留下原成功候选。该保护逻辑在此帧正确执行了保守契约，真正可修的是上游角点取证过严。

## 闪烁因果与全局代码只读核对

112 帧形成 96 段连续漏检：83 段为单帧，11 段为两帧，另有一段三帧、一段四帧；95 段前后均为原日志 DETECTED，其中 82 个单帧漏检位于成功帧之间。见 [flicker_sequence.json](evidence/final-fixes/fix2-sweep/flicker_sequence.json)。这些已不是仅凭观感推断的“可能闪烁”：当前帧检出状态确实反复出现有→无→有。

调用链为：角点取证失败 → decode 当前 detections 为空（382 为竞争提前拒绝）→ TemporalStabilizer 的 EMPTY_CURRENT 清除平滑状态并返回 NOT_DETECTED → debug_renderer 仅在当前状态 DETECTED 时画 raw/stable 框。原配置 display_hold_enabled=0，而且该层的显示历史不是缺失几何测量的替代来源。因此漏检既使当帧框消失，也打断下一帧的连续平滑，是这段视频闪烁的已证实重要原因。不能用显示历史填框来代替修复当前帧证据。

| 只读代码位置 | 已核实的作用 | 对应判断 |
| --- | --- | --- |
| geometry_observation.cpp、geometry_l_topology.cpp、geometry_anchor_evidence.cpp、geometry_matcher.cpp | 工作图实际 L 拓扑、合法凹锚点和三 L 组合 | A 无法组成三 L；C/D 已通过，不扩大此层搜索 |
| geometry_validation.cpp:292–306 | 绑定组件碰边则 CLEARLY_INCOMPLETE | B 的正确拒绝来源 |
| geometry_assignment_completion.cpp:40 | 拒绝明确不完整父假设，随后才匹配 M/S | B 不是 M/S 阈值问题；C/D 已完成 |
| corner_observation.cpp:11–45 | 原图轮廓双向映射、来源唯一、原边界截断检查 | 112 帧来源均可取得，不改分割或映射预算 |
| corner_edge_fit.cpp:29、95–110 | 平均残差/跨度门控与指定有限边位置、方向门控 | R1 与 R2 的实际阻断点 |
| corner_edge_fit.cpp:124–150 | 连续连接、凸性、交点及有限延伸 | 保留；它们会放大被早期删掉长弧的后果 |
| corner_resolver.cpp:49–80 | 按物理身份取四角，首个失败即停止 | 需要整帧复算，不能只修首个角 |
| decode_stage.cpp:33–69、semantic_resolver.cpp | 保留竞争、拒绝未解竞争、几何/方向归并 | 382 需把两条候选都取证成功 |
| corner_evidence_validation.cpp、detection_validator.cpp、detection_publication.cpp | 重算观测证据、四边形约束、float 发布 | 重试必须用同一批准预算完成全套验证 |
| temporal_stabilizer.cpp:73–75、app/debug_renderer.cpp:56–76 | 空当前测量打断平滑，未检出当帧不画框 | 检测失败到显示闪烁的直接链路 |
| config/detector.yaml:39、72；detector_verification.yaml:39、72 | 两个正式配置当前均为残差 0.5、位置 9 | 后续配置需要统一，不只改一个运行模式 |

## 修改范围、定位精度与性能判断

直接统一放宽两个现有预算是最小的配置改动，且离线试验的整帧结果为 1088 检出、588 未检出，方向无新增变化。但它重新参与原本成功帧的候选选择：原 976 成功帧中 55 帧角点发生变化，最大 1.59944 px。这个数是两次输出的差，不是人工角点定位误差。用户本次标的是有/无，没有标精确角坐标，不能据此宣称 2 px 真值验收通过。

全局放宽还会增加可参与配对的支持弧。离线单线程工具累计 decode 计时约为原预算的 1.83 倍；把重试仅限于 C/D 的粗略成本估计约为原预算的 1.27 倍。这两个重放进程曾并行运行，且工具冻结上游，数值仅提示搜索成本取舍，不是正式 Release 性能基准。

因此推荐先保留原预算的成功路径，对明确的角点取证失败整帧做一次受控重试。此设计可保持原 976 个成功帧 raw 输出，避免全局更换预算带来的 55 帧额外变化与大量无必要的重搜索。检测新增后时序轨迹仍会自然变化，不能要求 tracks 与旧日志逐位相同。

## 具体有效可执行修改方案（供用户审核，当前未实施）

### 推荐方案：原预算优先，角点证据失败才重试一次

1. **显式配置一份重试预算。** 保持基线 max_line_fit_error=0.5、max_edge_position_distance_px=9.0；新增可选 corner 重试配置，获批后开启，重试值为 0.75、9.5，其余预算从基线完整继承。0.75 有覆盖本组约 0.604 px 实拍残差的余量；9.5 只补 784 的位置边界余量，不采用大范围 15 px。这些是本视频实验支持的候选值，正式精度验证后才能定稿。

   预计涉及 include/mark/detector_config.hpp 的内部配置结构、lib/config/config.cpp 的严格字段读取/校验/导出，以及 config/detector.yaml、config/detector_verification.yaml 的一致配置。新可选项缺省关闭，旧配置行为不变；负值、非有限值、比基线更小且无解释的重试预算明确报配置错误。

2. **在 decode 编排层复用一次完整候选求解。** 在 lib/pipeline/decode_stage.cpp 抽取无状态的单次求解函数，输入当前 PreparedFrame、完整 completed batch、模型、指定 CornerConfig。第一遍原预算正常返回 DETECTED 就直接提交。仅在本帧 NOT_DETECTED、未搜索截断、已有完整候选，且失败诊断包含明确的角点 NO_VALID_ADJACENT_EDGES 时启用第二遍；A/B 这种没有 completed 候选的情况不触发。

   第二遍使用同一批当前帧完整候选，不重估三 L 父仿射，不重新补 M/S，不删失败竞争候选，不读取历史图像或角点。四角取证、屏幕排序、证据校验、语义竞争及 float 发布全部使用重试配置，第二遍仍失败则保持 NOT_DETECTED。每帧最多一次重试，资源截断仍保守拒绝方向声明；不按帧号或 A/B/C/D 人工标签写特判。

3. **把预算来源写进可复核诊断。** 记录是否重试、触发角/候选、原失败原因和实际使用的预算配置。只提交最终一次结果的 measurements/detections，避免两次调用污染 counts、候选编号、事件或导出载荷。配套内部 diagnostics context/recorder 和独立校验工具能够用相同配置复算最终证据；不能用 0.5 的旧校验预算拒绝实际由 0.75 合法获得的证据，也不能靠关闭 validator 放行。

4. **增加针对真实原因的回归用例。** 从已归档原图与诊断提取连续像素轮廓及模型边，建立可重复 fixture，至少覆盖：21 的一种边缺失、7 的连接拒绝、4 的延伸拒绝、784 的 9.00930 px 位置临界、382 的一成功一失败竞争。检查原预算失败、批准重试通过、来源/跨度/连接/凸性/最终发布均仍满足契约；另测重试关闭、原成功不重试、一次重试仍失败、搜索截断不重试。保留真实截断片、缺 M/S、长倒角远交点、平行边、内侧平行边、伪造/跨帧证据等拒绝用例的目的。

   预计修改 tests/corner_edge_fit_test.cpp、corner_resolver_test.cpp、observability_integration_test.cpp、config_contract_test.cpp/config_roundtrip_test.cpp 等相关测试；如需新增测试入口，按正式方案确定 CMake 与测试数量。当前已有 25 项应全过，不能因本次 fixture 引入而删掉原用例目的。

5. **正式验收与收尾。** 先冻结当前源代码和配置、旧 976 个成功结果、700 行人工真值。修改获批后，重新执行完整 Debug/Release 视频流程，核对全 1676 帧：C=106、D=6 全检出；A=564、B=24 全未检出；原 976 成功帧无丢失，推荐方案要求这 976 帧 raw 角点/方向保持基线；382 两候选取证成功后自然归并，竞争保护仍在。增加对应角点真值/独立人工精度核验，测正式耗时及重试次数，检查 diagnostics profile、导出 counts 与 actual pipeline 一致。

   新视频证据只写 fix2-sweep/ 的新验证子目录，原 Path A 封存档案保持原样；更新本报告实际结果，清理本次自建 build。具体代码修改清单、预算数值及实施仍以用户审核为准。本轮没有实施这些改动，也没有把离线实验结果写成正式验收通过。

### 较小备选：直接修改两个全局配置数值

若用户更看重最小代码改动，可统一将两个运行配置中的 max_line_fit_error 改为 0.75、max_edge_position_distance_px 改为 9.5，算法代码不动。离线实验已证明该组合恢复本组 112 帧且不新增 A/B 误检；正式执行前仍需接受原成功帧 55 个角点结果变化、候选搜索开销增加，并完成角点精度和全流程性能验收。无论选择哪一种，不需要放宽 M/S 匹配、取消边界拒绝、删除竞争保护或开启历史角点补框。
