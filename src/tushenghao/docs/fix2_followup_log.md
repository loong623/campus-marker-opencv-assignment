# Fix2-sweep 后续小修施工记录

状态：完成；功能、排版、双配置测试、视频回归、README 实操、归档和本轮 build 清理均通过。
开始：10-07 00:32:22（Asia/Shanghai）。

依据：[执行方案](ref/Fix2-sweep_后续小修_Codex执行方案.md)。审阅分支/提交由方案提供：feat/final-fixes @ bab614ac74b2a3ddc54097cb912a1d2dfd028a5c；未运行 Git，不声称当前工作树精确等于该提交。实际起点已保存 139 个源码/文档快照及 1784 个保护输入哈希，相关函数与方案问题吻合。

## 执行顺序

0. 保存当前源码、原 README、配置/ref/公共头/path-a/原 fix2-sweep/视频哈希；三个指定 build 开工前不存在。
1. 新建修改前 Release/Debug，分别 25/25；生成本轮同配置 1676 帧基线并核查完整性。C sample 只复用原生成器的 4 张小样，三尺度共 12 条修改前测量。
2. F01/F01-C：工具唯一坐标计数、原访问数单列、完整弧不变，helper 纳入测量代码 hash。C++ 七类主动检查、Python 10 点边界及小样指标对照通过。
3. F02：生成器标记 -1，验证层按原 pointPolygonTest 部件均值最大值回填；原 > 边界和面积/完整性分支保持。G01–G08 及独立真实父假设统计通过。
4. 功能版本 Release/Debug 均 25/25；README 改三步入口，历史全文迁移，41 个相对链接修正后能逐字还原原文。
5. 保存功能版本的排版前清单/快照，仅格式化 83 个有长行或单行多语句的自有文件；Allman/4 空格/100 列，decode_stage/run_report 另加逻辑段空行。Clang token/宏逻辑结构通过；双配置各 25/25。
6. 最终同视频回归、README 三模式各 1676 帧、实际 display 启用、视频全读回通过。全部 1114 个已验证父假设独立算式与字段差为 0；1090 个补全分支继承父范围。
7. 审校生成器旧注释，修正“规则未冻结”措辞；仅注释、token 不变，但为同步最终源码字节指纹重新构建、双配置 25/25、完整 1676 帧比较。最终归档后清理已完成。

## 改动与保留

支持点 helper 使用精确 (x,y) 集合，不四舍五入、不按 epsilon 合并，拒绝非有限坐标；两弧各自计数，输入不改。Python 从原弧独立核查，不相信 support_points 自述。生产 fitLine 的全部访问序列、权重、均值分母与各冻结预算不变。
validation_residual 单位为工作图 px；统计是本次已验证部件中，投影顶点到完整观测轮廓平均距离的最大值。未计算为 -1，真实 0 可成立。它不参与排序/置信度、不表示预测不确定度；默认初始化及公开布局未改。
格式化未改标识符、运算符、字面量、include/using 顺序、宏逻辑行及函数式宏邻接语义；不拆函数、不改循环/排序/截断。文件及理由见[清单](evidence/final-fixes/fix2-followup/style/manifest.json)。

## 执行中的失败与处理

- 临时开发工具第一次联网被沙箱拒绝，保留失败日志；经网络授权后安装到 /tmp，正式运行依赖不变。
- 初次 Clang token 检查发现默认 SortUsingDeclarations 重排了两个 using。恢复排版前快照，关闭此选项，全部 83 文件重新检查通过；未进入验收前保留该差异。
- 独立 hash 核对初次按规范化路径排序，与 CMake 含 ../.. 的真实字符串排序不同；随后直接复用实际 CMake 哈希段和文件序列，与测量字段完全吻合。
- 严格结果核对明确允许 parent trace 的 residual 数值变化；其它文字及字段仍完全相等，未改正式比较器过滤规则。
- 为保护用户已有 build/release 与统一归档，README 实操仅替换构建/输出目录，视频占位符换为已确认的绝对路径；运行选项一致。依赖已满足，未重复 apt 安装。

## 实际命令耗时

以下为真实子命令起止与耗时，包含构建、视频处理和核查；并行执行不可相加为总用时，桌面授权等待不在子命令运行耗时内。完整 argv、退出码与日志见[commands.md](evidence/final-fixes/fix2-followup/commands.md)和[commands.jsonl](evidence/final-fixes/fix2-followup/commands.jsonl)。

| 动作 | 开始（上海） | 结束 | 秒 | 退出码 |
|---|---|---|---:|---:|
| baseline-release-configure | 10-07 00:32:22 | 10-07 00:32:23 | 0.905 | 0 |
| baseline-fixture-configure | 10-07 00:32:22 | 10-07 00:32:24 | 1.576 | 0 |
| baseline-release-build | 10-07 00:32:55 | 10-07 00:33:21 | 26.331 | 0 |
| development-tools-install | 10-07 00:33:15 | 10-07 00:33:23 | 8.122 | 1 |
| baseline-fixture-build | 10-07 00:33:15 | 10-07 00:33:37 | 22.245 | 0 |
| development-tools-install-network | 10-07 00:34:24 | 10-07 00:34:37 | 13.081 | 0 |
| baseline-video | 10-07 00:34:25 | 10-07 00:36:38 | 132.831 | 0 |
| baseline-release-tests | 10-07 00:34:25 | 10-07 00:34:27 | 1.828 | 0 |
| baseline-debug-configure | 10-07 00:34:43 | 10-07 00:34:44 | 0.957 | 0 |
| calibration-sample-before | 10-07 00:35:31 | 10-07 00:35:32 | 0.886 | 0 |
| baseline-debug-build | 10-07 00:35:31 | 10-07 00:35:51 | 20.080 | 0 |
| calibration-measure-before | 10-07 00:35:42 | 10-07 00:35:42 | 0.238 | 0 |
| baseline-check-run | 10-07 00:36:49 | 10-07 00:36:50 | 0.570 | 0 |
| baseline-debug-tests | 10-07 00:36:49 | 10-07 00:36:58 | 9.320 | 0 |
| f01-release-build | 10-07 00:38:22 | 10-07 00:38:23 | 1.546 | 0 |
| f01-fixture-build | 10-07 00:38:22 | 10-07 00:38:27 | 5.418 | 0 |
| f01-python-tests | 10-07 00:38:22 | 10-07 00:38:22 | 0.031 | 0 |
| f01-cpp-tests | 10-07 00:38:48 | 10-07 00:38:48 | 0.073 | 0 |
| calibration-measure-after-count | 10-07 00:38:48 | 10-07 00:38:48 | 0.209 | 0 |
| f01-small-C-comparison | 10-07 00:39:14 | 10-07 00:39:14 | 0.039 | 0 |
| f02-release-target-build | 10-07 00:40:41 | 10-07 00:40:47 | 6.242 | 0 |
| f02-release-final-target-build | 10-07 00:41:34 | 10-07 00:41:36 | 1.117 | 0 |
| f02-active-tests | 10-07 00:42:51 | 10-07 00:42:51 | 0.023 | 0 |
| functional-release-build | 10-07 00:43:10 | 10-07 00:43:25 | 15.240 | 0 |
| functional-debug-build | 10-07 00:43:10 | 10-07 00:43:26 | 15.767 | 0 |
| functional-release-tests | 10-07 00:44:00 | 10-07 00:44:02 | 1.828 | 0 |
| functional-debug-tests | 10-07 00:44:00 | 10-07 00:44:09 | 9.259 | 0 |
| style-format | 10-07 00:48:30 | 10-07 00:48:30 | 0.356 | 0 |
| style-token-check | 10-07 00:48:30 | 10-07 00:48:36 | 5.738 | 1 |
| style-format-preserve-order | 10-07 00:49:38 | 10-07 00:49:38 | 0.254 | 0 |
| style-token-check-final | 10-07 00:49:38 | 10-07 00:49:45 | 6.806 | 0 |
| style-token-check-sections | 10-07 00:50:11 | 10-07 00:50:18 | 6.745 | 0 |
| style-debug-build | 10-07 00:50:58 | 10-07 00:51:22 | 24.289 | 0 |
| style-release-build | 10-07 00:50:58 | 10-07 00:51:26 | 28.427 | 0 |
| style-fixture-build | 10-07 00:50:58 | 10-07 00:51:20 | 22.115 | 0 |
| independent-geometry-build | 10-07 00:50:58 | 10-07 00:51:04 | 5.965 | 0 |
| style-release-tests | 10-07 00:52:22 | 10-07 00:52:24 | 1.897 | 0 |
| style-debug-tests | 10-07 00:52:22 | 10-07 00:52:32 | 9.328 | 0 |
| final-calibration-measure | 10-07 00:52:22 | 10-07 00:52:23 | 0.306 | 0 |
| readme-check-detector_verification | 10-07 00:52:23 | 10-07 00:52:23 | 0.057 | 0 |
| readme-check-detector | 10-07 00:52:23 | 10-07 00:52:23 | 0.061 | 0 |
| readme-check-detector_debug | 10-07 00:52:23 | 10-07 00:52:23 | 0.055 | 0 |
| verification-after | 10-07 00:52:44 | 10-07 00:54:57 | 133.515 | 0 |
| readme-baseline | 10-07 00:52:44 | 10-07 00:54:58 | 134.787 | 0 |
| readme-debug-display | 10-07 01:04:57 | 10-07 01:07:20 | 142.981 | 0 |
| readme-verification-export | 10-07 01:04:59 | 10-07 01:07:29 | 149.907 | 0 |
| calibration-hash-audit | 10-07 01:06:52 | 10-07 01:06:52 | 0.024 | 0 |
| final-regression | 10-07 01:07:22 | 10-07 01:07:24 | 1.349 | 0 |
| final-check-run | 10-07 01:07:22 | 10-07 01:07:23 | 0.582 | 0 |
| independent-geometry-residuals | 10-07 01:07:22 | 10-07 01:07:23 | 1.056 | 0 |
| readme-export-video-readback | 10-07 01:09:39 | 10-07 01:09:42 | 3.448 | 0 |
| readme-verification-check-run | 10-07 01:11:36 | 10-07 01:11:37 | 0.538 | 0 |
| readme-debug-check-run | 10-07 01:11:36 | 10-07 01:11:37 | 0.539 | 0 |
| final-comment-release-build | 10-07 01:14:43 | 10-07 01:14:51 | 8.072 | 0 |
| final-comment-debug-build | 10-07 01:14:43 | 10-07 01:14:50 | 7.747 | 0 |
| final-comment-token-check | 10-07 01:14:43 | 10-07 01:14:52 | 9.001 | 0 |
| final-comment-fixture-build | 10-07 01:14:43 | 10-07 01:14:52 | 9.132 | 0 |
| final-review-release-tests | 10-07 01:16:07 | 10-07 01:16:09 | 1.888 | 0 |
| final-review-debug-tests | 10-07 01:16:07 | 10-07 01:16:16 | 9.349 | 0 |
| final-review-measure | 10-07 01:16:07 | 10-07 01:16:07 | 0.300 | 0 |
| verification-final-review | 10-07 01:16:39 | 10-07 01:18:49 | 129.981 | 0 |
| calibration-hash-final-review | 10-07 01:18:04 | 10-07 01:18:04 | 0.012 | 0 |
| final-review-check-run | 10-07 01:19:49 | 10-07 01:19:49 | 0.537 | 0 |
| final-review-regression | 10-07 01:19:49 | 10-07 01:19:50 | 1.270 | 0 |
| final-review-geometry-residuals | 10-07 01:19:49 | 10-07 01:19:50 | 0.969 | 0 |

## 剩余范围

本轮没有未完成项；没有扩展新的逻辑问题修复。C/D 漏检、闪烁、成像模型和通用预算推广仍属于独立后续工作，本轮不宣称解决。没有新增人工角点精度或播放器视觉审核；旧干净环境未验证记录不被覆盖。验收见[独立验收表](fix2_followup_acceptance.md)。

收尾时间：2026-10-07 01:26:52（Asia/Shanghai），截至此时实际墙钟 54.50 分钟，包含工具/桌面授权等待和证据检查；不是算法运行耗时。仅删除 build/fix2-followup-release、debug、fixture；清理前缓存/日志已封存，见[清理记录](evidence/final-fixes/fix2-followup/cleanup.json)。

末尾动作 final-scope-before-cleanup：UTC 2026-10-06T17:24:22.141963+00:00 → 2026-10-06T17:24:26.977838+00:00，4.836s，退出码 0。

末尾动作 cleanup-builds：UTC 2026-10-06T17:26:09.906131+00:00 → 2026-10-06T17:26:10.002464+00:00，0.096s，退出码 0。

末尾动作 final-scope-after-cleanup：UTC 2026-10-06T17:26:52.951988+00:00 → 2026-10-06T17:26:56.766054+00:00，3.814s，退出码 0。
