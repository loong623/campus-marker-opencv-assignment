# 冻结v2验收结果与范围冲突（2026-10-06）

用户已批准C-v6三项修正，生产配置和冻结v2仅改方向差1→3°、原图边位置2→9px、M/S工作图边界3.5→9.5px。全部其他数值/规则不变；模型坐标/面积、原三L算法及参数未改。V标注按用户决定暂时跳过，commit由用户执行。

正式H退出2（验收FAIL）：阶段689/720；无噪声138/144，噪声551/576，隔离720/720；错误有效几何/方向0。独立核查屏幕顺序、bbox及支持证据差异均0，最大真值误差1.346291202原图px。门槛仍为144/144、571/576以及每噪声层95/96，没有降低。

[完整分层报告](H-video-v2.md)和[31条失败](H-video-v2.failures.json)保留全分母。只读诊断入口`block3_verify --diagnose-stage <fixture目录> <case_id> <冻结配置> <工作宽>`逐父逐候选计算原始指标，不修改门限，不向正式链路传真值。[全部31条诊断](H-v2-all-failure-diagnostics.jsonl)确认：

| 分类 | n | 具体反例 | 定位 |
|---|---:|---|---|
| M拓扑没有可用固定epsilon | 16 | H-r0-a60-s0-w480 | 真实M边界差2.050109工作px、相对面积差0.016559829，均在预算内；五固定比例均不能给出指定顶点数/凹凸结构。不是边界9.5不够。其他同类为480工作宽60°/300°，全表可查。 |
| 旧三L验证拒绝全部父假设 | 15 | H-r2-a45-s0-w1440 | generate返回6父，validate返回0，均报geometric residual too large；1440工作宽R2的45°/135°/315°×5噪声条件。后续补全没有父输入。 |

这两类不能通过把H误差放回C统计、调宽预算或补真值父假设消除。终稿§12明确“不得修改旧三L逻辑和参数”，§9要求出现范围外上游反例时交回review，因此当前不能在原范围内承诺全部闭合。需要单独review M/S拓扑规则，以及是否扩展旧三L验证修复范围；现有批准预算继续冻结。

全视频退出0，0..1675共1676/1676记录，Detection帧0、资源截断0。包括1200：实际三L父存在，M/S补全输出1分支，原图P0连续边证据拒绝；没有用1200调参。pipeline mean1.774ms、P99 4.948ms、max7.274ms，单次本机运行不含视频解码。0检测不能证明召回/负例误检通过；V暂跳、N/U/O无确认标签、Q无人工真值、原32标注/封存baseline未提供，相关指标保持null/未验证。

关联/资源测量及数值审批已闭合，证据见预算表C-v1..v6与resources-v4（8场景，128候选/4096扩展/64输出，截断不唯一）。完整H验收未闭合；资源有界不等于硬实时保证，64分支decode约200ms。

## 验证记录

| 命令 | 退出码/结果 |
|---|---|
| `build/block3-repair/marker_app --check-config src/tushenghao/config/detector.yaml` | 0 |
| `build/block3-fixture/block3_verify build/block3-H-v1 src/tushenghao/docs/evidence/block3/frozen_detector_v2.yaml build/block3-H-verify-v3.jsonl` | 2；689/720、正式FAIL |
| `build/block3-repair/decode_audit data/raw/marker_video.avi all src/tushenghao/docs/evidence/block3/frozen_detector_v2.yaml` | 0；1676/1676、0Detection |
| `python3 src/tushenghao/tools/block3_fixture/report_verification.py build/block3-H-verify-v3.jsonl build/block3-video-v3-budget-v2.jsonl src/tushenghao/docs/evidence/block3/H-video-v2.md` | 0；归档成功，报告H_acceptance=false（不是验收成功退出） |
| `ctest --test-dir build/block3-repair --output-on-failure` | 0；15/15 |
| `ctest --test-dir build/block3-clean --output-on-failure` | 0；15/15 |
| `cmake --build build/block3-fixture --target block3_verify -j 4` | 0；新增只读失败分类诊断 |
| 新只读diagnose-stage：480宽真实反例/123非法工作宽 | 0/1；6生成父、2验证父及输入拒绝符合预期 |
| `git diff --check` | 0 |

输入/配置/代码/结果hash和1200全记录见[provenance](final-run-provenance-v2.json)。正式H/video在新增只读诊断前运行，各自代码hash随结果保存；新增诊断工具没有改变Detector代码，故未无理由重复全量。原始较大PNG/JSONL在build，删除前须归档；版本化摘要/全部失败诊断在docs随代码保存。

整体Block3仍WIP，公共process仍NOT_READY。模型绑定配置化、完整§9.1/一般仿射与更小目标支持范围等原有遗留事项不因本轮15/15测试自动关闭。HEAD25b6cbe未变，没有git add/commit。
