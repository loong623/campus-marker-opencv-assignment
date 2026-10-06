# fix2-sweep 分析证据

700 行人工标签来自用户按类别给出的整体复核结论，登记于 `manual_label_authorization.json`。A/B 为无，C/D 为有；`index.csv` 是当前人工索引。

当前状态见 `analysis_manifest.json`，逐帧结果见 `root_cause_by_frame.csv`，统计与限制见 `analysis_summary.json`，主报告为 [fix2_sweep_report.md](../../../fix2_sweep_report.md)。`manifest.json`、`artifact_hashes.json` 是首次导出时的历史快照；其中 index、报告对应的是当时未标注版本。当前产物校验见 `analysis_hashes.json`。

分析工具没有修改正式算法、配置或 Path A 档案。独立编译产生的三个执行文件已清理，源码和编译/运行日志保留。没有创建项目 build。

## 方法与重现

在仓库根目录运行，所有输出位置应使用本目录下的新子目录，避免覆盖已经归档的分析结果。

1. `tools/replay_targets.txt` 记录 112 个原日志首个失败角的组件轮廓、父仿射和物理角。`tools/replay_corners.cpp` 调用生产原图来源匹配和边拟合，独立工具副本仅收集弧身份及最终门控前的统计；副本出处与变更范围见 `tools/instrumentation_provenance.json`。每次调用先断言副本诊断与未改生产函数一致。
2. 编译 `tools/replay_corners.cpp` 时，添加 `-std=c++17 -O2 -Isrc/tushenghao/lib -Isrc/tushenghao/include`，链接原 `lib/corners/corner_observation.cpp`、`corner_edge_fit.cpp` 及 `pkg-config --cflags --libs opencv4` 给出的选项。程序参数依次为：本证据目录、原 Path A effective_config.yaml、原 marker_geometry.yaml、replay_targets.txt。标准输出是 JSONL。
3. `corner_replay.jsonl` 是最初只采集统计的基线；`corner_sensitivity.jsonl` 保存 112 个角点的十组敏感性实验。当前工具还增加了六组精细位置实验，单独运行 `tools/position_targets.txt` 得到 `position_sensitivity.jsonl`。所有变化仅存在工具内存中的配置副本。
4. `tools/full_replay_targets.txt` 保存全部 1676 帧的已完成候选与工作图组件。`tools/full_frame_replay.cpp` 顺序解码原视频，并调用原角点 resolver、screen order、corner evidence validation、detection validator、semantics、float publication。它复现 decode 的无状态求解与竞争保护，不调用正式 Detector/时序/渲染，不重跑上游候选生成。
5. 完整重放编译需链接 `lib/corners/` 中的 `corner_resolver.cpp`、`corner_observation.cpp`、`corner_edge_fit.cpp`、`corner_evidence_validation.cpp`、`detection_validator.cpp`、`screen_order.cpp`、`semantic_resolver.cpp`、`detection_publication.cpp`，其余编译选项同上。程序参数依次为：原视频、原 effective_config.yaml、原 marker_geometry.yaml、full_replay_targets.txt。缺省输出原预算与残差 0.75 的两次结果；额外参数 `9.5 probe-only` 输出残差 0.75、位置 9.5 的结果，避免重复计算原预算。
6. 原始运行参数使用 `src/tushenghao/docs/evidence/final-fixes/path-a/verification-release/effective_config.yaml` 和 `src/tushenghao/config/marker_geometry.yaml`，视频为 `data/raw/marker_video.avi`。所有工具读取原配置，不写配置。
7. `tools/summarize_analysis.py` 逐帧比对基线与原始日志（状态、测量数量、976 个 raw 角点及方向），核对 C/D 恢复与 A/B 拒绝，生成逐帧根因表和汇总，同时检查初始清单中的 1315 个受保护文件哈希。此脚本运行会更新本目录中的汇总文件，归档后需要同步更新当前哈希清单。

本次四角重放证明了预算门控对本视频漏检的因果关系，不等于正式修复已实施，也不能替代后续角点真值精度验证、全流程 Debug/Release 运行和正式性能验证。
