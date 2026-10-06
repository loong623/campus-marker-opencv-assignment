# 文档与工具整理证据

本批只验证最终目录/导航及工具可运行性，不是新算法版本。说明见[整理报告](../../architecture/docs-tools-organization.md)。

| 证据 | 内容 |
|---|---|
| [verification.json](verification.json) | 两套全量构建、16测试、正式H、Python与默认配置运行 |
| [integrity.json](integrity.json) | 原文件存在性、冻结/参考完整性、算法源hash与既有gzip校验 |
| [links.json](links.json) | README、全部docs和工具Markdown的本地目标及锚点扫描 |
| [H原始结果](H-final-layout.jsonl.gz)、[归档摘要](H-final-layout-archive.json) | 本轮正式H720条；与冻结v7除代码SHA/耗时外等价 |
| [独立报表JSON](final-docs-tools-independent-report.json)、[失败列表](final-docs-tools-independent-report.failures.json) | 本轮H与既有v7全视频结果的独立核查 |
| [默认审计](final-docs-tools-audit-default.jsonl)、[/tmp审计](final-docs-tools-audit-tmp.jsonl) | 无显式配置参数的帧0/1200及/tmp帧0 |

## 构建与运行日志

- [主工程配置](logs/final-docs-tools-configure.log)、[全量构建](logs/final-docs-tools-build.log)、[16项CTest](logs/final-docs-tools-ctest.log)
- [工具配置](logs/final-docs-tools-fixture-configure.log)、[工具全量构建](logs/final-docs-tools-fixture-build.log)
- [应用配置检查](logs/final-docs-tools-app-check.log)、[/tmp配置检查](logs/final-docs-tools-app-check-tmp.log)
- [正式H](logs/final-docs-tools-H.log)、[Python报告运行](logs/final-docs-tools-python-report.log)
- [审计stderr](logs/final-docs-tools-audit-default.log)、[/tmp审计stderr](logs/final-docs-tools-audit-tmp.log)

新旧路径及整理前完整hash清单见[整理manifest](../../architecture/docs-tools-organization-manifest.json)。
