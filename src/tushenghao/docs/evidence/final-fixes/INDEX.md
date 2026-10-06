# Final-fixes 永久证据

按批准终稿 A→B→C→D 实施。代码与自动验证完成，待用户人工 review。
G-DISPLAY（有屏显示及退出）和 G-VIDEO（播放器）待人工确认；
G-CLEAN 按用户明确决定记未验证，本机依赖/命令审查及新路径复现已完成。

| 入口 | 内容 |
|---|---|
| [验收报告](../../final-fixes_acceptance.md) | 13 项矩阵、验证、踩坑与已知限制 |
| [执行命令](archive/commands.json) | 实际 argv、CWD、时间、退出码、完整 stdout/stderr 路径 |
| [清理凭证](archive/cleanup.json) | 所有权校验、实际删除路径及保留的旧 build；执行后填写结果 |
| [产物 SHA256](archive/artifact-manifest.json) | 永久产物大小及 hash；排除清单自身，避免自引用 |
| [修改前清单](environment/prechange-manifest.json) | 分支/起点、输入/golden、源码/公共头/ref/历史证据、既有 build |
| [受保护文件核查](environment/protected-check.json) | 当前改动与禁止范围核查 |
| [配置算法核查](environment/algorithm-config-check.json) | portable 配置与冻结基准算法段相同 |
| [A 负例矩阵](batch-a/negative-matrix.json) | 空比较、短记录、重复/倒序、子集与全量 |
| [43 链接改动](batch-a/link-replacements.json) | Block4 旧路径与真实归档路径逐条对应 |
| [链接核查](batch-a/links.json) | 实际本地目标均存在 |
| [B Release](batch-b/ctest-release.stdout)、[B Debug](batch-b/ctest-debug.stdout) | Export 补记和 scope 隔离所在原有测试 |
| [C 拆分前后对账](batch-c/behavior-check.json) | 当前检测/失检/恢复、关闭计时、显式子集，去运行身份及实测耗时后全记录 exact |
| [C 可读性](batch-c/readability.json) | 拆分后的行宽与内部职责检查 |
| [最终 Release 测试](regression/ctest-release-label-final.stdout)、[最终 Debug 测试](regression/ctest-debug-label-final.stdout) | 原有 23/23，未增加/移走目标 |
| [baseline](regression/baseline/summary.yaml)、[baseline 控制台](regression/baseline.stdout) | 唯一 baseline 全视频性能测量及指纹，完整性核查通过 |
| [Release 全量比对](regression/compare-release.json) | 对 golden 的 1676 帧逐帧公共结果与原有语义原因核查，无 route-b |
| [Release 完整 run](regression/verification-release/) | frames.jsonl、export_timings.jsonl、manifest、effective config、summary |
| [完整 MP4](regression/verification-release/overlay.mp4) | 保留供人工播放器验收的唯一完整全视频 overlay |
| [视频读回](regression/video-readback.json)、[编码](regression/video-codec.stdout) | 1676 帧、1440×1080、70.408 FPS、codec=mjpeg、tag=mp4v |
| [Debug 全量比对](regression/compare-debug.json)、[完整 run](regression/verification-debug/) | 第二构建全视频结构回归 |
| [D 边界矩阵](app-output/matrix.json) | 无屏/坏 DISPLAY、计时关闭视频、CLI 负例、失败/partial、中文摘要 |
| [损坏视频 partial](app-output/truncated/overlay.partial.mp4) | 解出 41 帧后不完整，未发布 overlay.mp4 |
| [新路径结果](clean-linux/result.json)、[新路径测试](clean-linux/ctest-label-final.stdout) | 仅复制源码/输入，重新构建；23/23，三配置/三模式 10 帧；模型 hash 相同 |
| [新路径环境口径](clean-linux/path-setup.json) | 使用本机依赖，不能声称干净 Ubuntu PASS |

完整输入 SHA256 为 aa1219a7a7b702ea1265be8752853a267c982a651afa0f7846f516f9517c9ac7。
1676 帧、864 DETECTED/812 NOT_DETECTED、FNV1a64 c1785fb03a54ed25。
golden 保留在 [Block5 原位置](../block5/step0/patched-baseline.jsonl)，原 SHA256 不变。

run manifest 中 source/model/build 路径是当时实际运行事实，保持原样。
短输入与损坏输入同时归档于 [inputs](regression/inputs/)，历史 manifest 不因复制而改写；
因此这些复制 run 不是可在任意目录直接 check-run 的自包含包。
验收时已在源输入仍存在期间完成检查；完整 run 的 source 是仓库原视频。
复核可按 README 重新构建，用明确输入和新输出目录重新运行。

有屏人工验收：按 [README](../../../README.md) 的 debug --display 命令运行，确认弹窗、
连续显示、empty 不画旧框、unknown 不补标签；q/ESC 后应非零退出并产生 INCOMPLETE.json。
播放器验收：打开上面的完整 MP4，确认可以播放且时间轴连续。上述两项尚未填写 PASS。
历史 ref 与 Block3/4/5 证据未修改；清理仅针对本轮有精确所有权标记的三个 build 及临时副本。
