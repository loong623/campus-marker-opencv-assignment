# 文档与工具整理验证

本轮按批准方案移动180个文件，建立文档、工具及证据批次索引。README和block3.md改为现行目录与状态入口，完整Block3过程另存历史页。没有删除文件、修改算法或预算；未commit，交用户review。

## 阅读顺序与目录

从[README](../../README.md)进入[文档索引](../INDEX.md)，查当前[Block3状态](../block3.md)、[预算审批](../block3_budget.md)和[适用边界](../evidence/block3/05-L-observation-v5-v7/适用边界与已知局限.md)。历史数据不与当前结论混读。

| 位置 | 内容及规则 |
|---|---|
| docs/根目录 | 当前Block记录、预算与复用说明，INDEX为入口 |
| [architecture/](INDEX.md) | include分析、源码迁移、此次整理；历史分析JSON保留记录时的路径 |
| [evidence/block3/](../evidence/block3/INDEX.md) | 01契约、02校准、03基线v1–v2、04四项修复v3–v4、05三L观察v5–v7；各批次有INDEX，日志放logs/ |
| [evidence/layout-reorganization/](../evidence/layout-reorganization/) | 前轮源码搬迁的构建及回归证据 |
| [evidence/docs-tools-organization/](../evidence/docs-tools-organization/INDEX.md) | 本轮链接、完整性、构建、测试与工具运行验证 |
| [history/](../history/block3-before-final-organization.md) | 整理前完整block3.md，仅供过程对照 |
| [ref/](../ref/) | 三份冻结依据，路径与内容均保持 |
| [tools/](../../tools/INDEX.md) | audit、common、block3_fixture的generation/calibration/validation/diagnostics/reporting；synth_ref整包保持原位 |

各批次中的原始数据、gzip与对应manifest共同迁移，原文件名不变。冻结v1–v7及候选v2共8个YAML保持在evidence/block3/原位置。新增本轮验证不冠以算法v8，不混入v7的原始结果。

现有非冻结Markdown除README/block3.md外仅修导航目标；历史命令、配置摘要与数值不重写为“本轮执行”。新INDEX给出当前构建和调用入口。README说明六个lib模块、include/mark、app、test_support与tools；block3.md中的现行源码描述已替换旧平铺路径。

## 完整性与构建

[整理manifest](docs-tools-organization-manifest.json)记录180组新旧路径与整理前1098个docs/tools文件的hash。原文件全部存在；160个非Markdown证据字节不变，3份ref、8个证据YAML、synth_ref整包及运行配置/模型YAML均保持。与上一轮迁移manifest核对，63个算法、接口、应用、测试及fixture源文件字节不变；21个既有gzip归档和解压原文均匹配原manifest。

工具CMake更新7个目标的源路径，目标名称和入口不变；主CMake更新审计源路径。decode_audit默认配置改由CMake给出绝对路径，避免移动后依赖错误的父目录层级。四个Python报表脚本共置reporting/，导入保持有效。

| 检查 | 结果 |
|---|---|
| 主工程全量Release构建（独立新build目录） | PASS |
| 独立fixture全量Release构建，7个工具目标 | PASS |
| CTest | 16/16，失败0 |
| 正式H全量重跑 | 阶段/隔离720/720，wrong_valid=0，PASS；除代码SHA/耗时外与冻结v7逐条相等 |
| 4个Python脚本编译与导入 | PASS；独立H/video报告核查H_acceptance=true |
| 默认配置运行 | 应用在仓库根和/tmp均PASS；审计帧0/1200及/tmp帧0与v7对应帧等价（除配置/代码SHA及耗时） |
| Markdown本地导航 | 可编辑文档失效链接0；冻结ref有6条原有缺失引用，详见下节及[链接扫描](../evidence/docs-tools-organization/links.json) |
| 文件完整性 | [完整性摘要](../evidence/docs-tools-organization/integrity.json)，无遗漏或证据内容改写 |

本轮未重跑全视频；独立Python报告读取已归档的冻结v7全视频1676条，审计另做选帧烟测。原全视频864检测帧的结论和适用范围保持，不能把目录整理或未标注检出数当作新的算法验收。

原始构建、CTest、H日志及运行输出见[本轮验证索引](../evidence/docs-tools-organization/INDEX.md)和[验证摘要](../evidence/docs-tools-organization/verification.json)。

## 冻结参考的既有缺失引用

只读扫描同时覆盖ref和synth_ref。冻结总汇总中的以下6个目标在整理前即未提供，不能通过改写冻结原文或创建替代冻结内容来声称链接全部有效。其原文件hash保持不变，可编辑导航无失效目标。

- `block0/白角标可用性统计.md`
- `板块1_接口与配置骨架_完整冻结文档.md`
- `板块2_几何原语_完整冻结文档.md`
- `板块3_语义映射_完整冻结文档.md`
- `板块4_输出稳定层_完整冻结文档.md`
- `板块5_可观测性_完整冻结文档.md`
