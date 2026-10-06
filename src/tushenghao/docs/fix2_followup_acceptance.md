# Fix2-sweep 后续小修验收

状态：完成；A01–A10 全部通过，本轮三个自建 build 已清理。执行依据：[后续小修方案](ref/Fix2-sweep_后续小修_Codex执行方案.md)。本轮修复测量/诊断口径、整理操作入口及排版，没有提高检出率、重定预算或解决闪烁。

## A01–A10 实测

| ID | 结果 | 实测与证据 |
|---|---|---|
| A01 | PASS | 配置、公共头、ref、path-a 与原 fix2-sweep 全部 1784 个保护文件未改；几何类型布局及默认初始化 token 不变，生成器唯一逻辑改动为 0.0 → -1.0 状态标记；[范围核查](evidence/final-fixes/fix2-followup/scope-check.json)。 |
| A02 | PASS | C++ 计数主动检查通过；原 C 小样 4 原图×3 尺度=12 条测量、96 条边，唯一数/访问数独立正确，全部原弧及非计数指标不变；[终版小样对照](evidence/final-fixes/fix2-followup/calibration/final-review-measurement-comparison.json)、[helper 哈希覆盖](evidence/final-fixes/fix2-followup/calibration/code-hash-final-review-check.json)。 |
| A03 | PASS | Python 从弧独立计数，10 次访问/9 唯一点不满足原 10 点门槛，10 唯一点满足；[边界检查](evidence/final-fixes/fix2-followup/calibration/python-count-check.json)。 |
| A04 | PASS | G01–G08 均通过；独立复算全部 1114 个真实已验证父假设，最大差 0.0；1090 个补全分支保留父统计；[逐项对照](evidence/final-fixes/fix2-followup/geometry/final-parent-residuals.jsonl)、[摘要](evidence/final-fixes/fix2-followup/geometry/final-parent-summary.json)。 |
| A05 | PASS | 基线、功能、排版及末尾注释审校四阶段，Release/Debug 每次均 25/25，8 次完整 CTest；终版 [Release](evidence/final-fixes/fix2-followup/tests/final-release.log)、[Debug](evidence/final-fixes/fix2-followup/tests/final-debug.log) 的主动检查在 Release 也实际执行。 |
| A06 | PASS | 原输入/同配置 1676 帧，0–1675 完整；四点、方向、bbox、tracks、display、时间戳及其它原因无回归，结果指纹 `0d2d2e63aef753bc` 与基线相同；[终版完整性](evidence/final-fixes/fix2-followup/check-run-final-review.json)、[终版比较](evidence/final-fixes/fix2-followup/regression-final-review.json)。 |
| A07 | PASS | 三配置检查、Release 构建、baseline/debug+display/verification+export 均实测；每种完整 1676 帧，display_active=true；视频读回 1676 帧/1440×1080/mjpeg；[七项对账及路径说明](evidence/final-fixes/fix2-followup/readme/quickstart-check.json)、[读回](evidence/final-fixes/fix2-followup/readme/video-readback.json)。 |
| A08 | PASS | 原 README 字节快照匹配，历史全文及代码块保留；只增提示并改 41 处相对链接，反向还原与原字节完全一致；[迁移证明](evidence/final-fixes/fix2-followup/readme/history-migration.json)。 |
| A09 | PASS | 83 个自有文件，Clang 原始 token 及预处理逻辑行/函数式宏邻接完全一致；[文件清单](evidence/final-fixes/fix2-followup/style/files.txt)、[理由与快照清单](evidence/final-fixes/fix2-followup/style/manifest.json)、[词法检查](evidence/final-fixes/fix2-followup/style/token-check.json)。 |
| A10 | PASS | 文档、历史、INDEX 和追加记录齐全；构建缓存、日志及 LastTest 已归档，本轮三个自建 build 已删除，用户已有 build/release 和旧证据保留；[清理记录](evidence/final-fixes/fix2-followup/cleanup.json)、[最终检查](evidence/final-fixes/fix2-followup/final-check.json)。 |

## 两项修复前后

| 支持弧例子 | 旧 support_points / 访问数 | 新唯一像素数 | 验证 |
|---|---:|---:|---|
| 空弧 | 0 | 0 | 主动检查 |
| 同坐标访问 5 次 | 5 | 1 | 输入序列不变 |
| 非相邻回走 6 次访问 | 6 | 3 | 精确坐标去重 |
| 两个紧邻但不同的有限坐标 | 2 | 2 | 无 epsilon 合并 |
| 两条弧各 3 次访问、2 唯一点 | 3 / 3 | 2 / 2 | 不跨弧合并 |
| C 小样首角两边 | 51 / 51 | 51 / 51 | 无回走，计数不变 |

非有限坐标明确报测量输入错误。新增字段只有标定 JSON 的 support_visits、support_points_counting，原 support_arcs 顺序与重复点完整保留。生产 fitLine 输入、权重、残差分母、转折裁剪及边绑定未改。历史 C-v5 无重复，计数修复不推翻原 0.5px 校准。

| 几何测量 fixture | 各部件均值（工作 px） | 独立最大值 | 验证后字段 |
|---|---|---:|---:|
| G01 顶点在轮廓上 | [0] | 0 | 0 |
| G02 轮廓外扩 1 | [1] | 1 | 1 |
| G03 三部件外扩 0/1/3 | [0,1,3] | 3 | 3 |
| G04 阈值=1 / nextafter(1,0) | [1] | 1 | 相等保留；略小拒绝 |
| G05 未计算输入 | [1] | 1 | 输入仍 -1，返回副本 1 |
| G06 面积/边界完整性分支 | [1] | 1 | 降级仍保留 1 |
| G07 缺失、空、非有限输入 | 不可计算 | 不伪造 | 无有效假设 |
| G08 非连续 ID、组件乱序 | [0,1,3] | 3 | 3 |

[测试侧独立数值记录](evidence/final-fixes/fix2-followup/geometry/unit-measurement-comparison.json)。真实视频字段最大值为 3.99816721385464 工作 px；它是已验证父部件的最大平均距离，不是三个锚点拟合误差、置信度、原图角误差或六部件统计。生成阶段 -1 与真实 0 已区分。

## 回归与指纹

1676 帧仍为 976 DETECTED / 700 NOT_DETECTED；只作回归计数，不代表召回率或正确率。全部 3423 个生成假设显示未计算状态，1114 个已验证父字段回填真实值，1090 个补全分支继承父统计。正式 observability_verify 比较器只排版，原过滤规则未改；附加独立核查只允许声明的 residual 字段和 parent trace 中残差数字变化，其余 details、events、counts、结果与时戳严格相等。

修改前源码指纹：`ec589deb31651c9acbd2e4b365ab620e825e7d87252979470dc7345be6e3d259`。终版源码指纹：`f15589d40618f4f2870d5b34904d7ae84f5ab234341624f9c32fa59d1866f72b`。输入/配置/ref/公共头 SHA 与实际源码清单见 [哈希与快照](evidence/final-fixes/fix2-followup/hashes-before.json)、[终版清单](evidence/final-fixes/fix2-followup/hashes.json)。review 分支/提交来自执行方案，未运行 Git，不把标签当工作树实际提交。

README 三模式在末尾仅注释审校之前完成，manifest 保留其实际源码指纹；[唯一差异的字节版本](evidence/final-fixes/fix2-followup/source-variants/geometry_matcher_before_comment_review.cpp) 已归档，C++ token 与终版一致。终版另跑完整视频并与修改前基线比较，通过。为保护已有 build/release，实操用本轮专用 Release build；示例输出换到本轮 readme 下三个新目录，其余运行选项一致。依赖已安装并经 CMake 实测识别，未重复 sudo apt 安装。

## 环境、排版与限制

Ubuntu 24.04 / GCC 13.3 / CMake 3.28.3 / OpenCV 4.6 / OpenSSL 3.0.13；完整版本见 [environment.json](evidence/final-fixes/fix2-followup/environment.json)。clang-format 18.1.8、libclang 18.1.1 仅安装在 /tmp 的开发期临时目录，不成为生产依赖。规则为 Allman/4 空格/100 列，禁短函数和短控制体、禁 include 与 using 重排、禁字符串切分，逻辑段另留空行；函数名及大函数职责未重构。

没有重标定 C/H、一般成像/透视泛化研究、置信度或桥接修改；本轮不能宣布解决 C/D 漏检或闪烁。未新增人工角点精度/播放器视觉复核；机器读回和实际 display 启用不冒充人工验收。原干净环境未验证的历史限制仍保留。未运行任何 Git 命令。

执行过程见 [施工记录](fix2_followup_log.md)；实际命令/耗时见 [commands.md](evidence/final-fixes/fix2-followup/commands.md)。
