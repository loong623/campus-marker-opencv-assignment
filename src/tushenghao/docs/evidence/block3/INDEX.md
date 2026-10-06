# Block3证据与版本索引

当前算法基准是[冻结v7](frozen_detector_v7.yaml)。先读[最终修复](05-L-observation-v5-v7/07-L-observation-repair.md)、[最终回归](05-L-observation-v5-v7/H-video-v7.md)及[适用边界](05-L-observation-v5-v7/适用边界与已知局限.md)。源码目录迁移仅改路径，输出与v7逐条相同。

## 版本对照

| 配置/实现 | H阶段正确 | 视频Detection帧/1676 | 状态与说明 |
|---|---:|---:|---|
| v1 | 完整门槛未通过 | 0 | 早期预算/弧分段阶段 |
| [v2](frozen_detector_v2.yaml) | 689/720 | 0 | 历史基线；关联口径修正后仍有拓扑/角取证阻断 |
| [v3](frozen_detector_v3.yaml) | 独立预算项对照 | — | 实拍角预算阶段，不冒称独立全链路最终版 |
| [v4](frozen_detector_v4.yaml) | 705/720 | 591 | 四项修复；三L观察设计仍阻断H |
| [v5](frozen_detector_v5.yaml) | 720/720 | 751 | 中间观察规则，额外epsilon闭合检查过严 |
| [v6](frozen_detector_v6.yaml) | 720/720 | 976 | 中间锚点方案，遗漏原轮廓部分实测凹点 |
| **[v7](frozen_detector_v7.yaml)** | **720/720** | **864** | **最终完整枚举；保留竞争拒绝，不追求最高检测数** |
| [源码目录重组](../../architecture/layout-reorganization-verification.json) | 720/720 | 864 | C720/H720/视频1676逐条与v7相同 |

C-v1…v6是测量实验版本，resources-v4/v5是资源测量版本，不与冻结配置版本号一一对应。H通过不代表总体Block3完成；视频未标注，检出数量不代表召回/正确率。

## 按实验批次查找

| 目录 | 主要内容 |
|---|---|
| [01-contract](01-contract/INDEX.md) | 初始契约红/绿记录 |
| [02-calibration](02-calibration/INDEX.md) | C统计、关联测量及候选pipeline |
| [03-baseline-v1-v2](03-baseline-v1-v2/INDEX.md) | v1/v2基线、H31失败、视频零检出根因、早期资源 |
| [04-four-track-v3-v4](04-four-track-v3-v4/INDEX.md) | 实拍角预算、支持弧、M/S、三L诊断、v4组合回归 |
| [05-L-observation-v5-v7](05-L-observation-v5-v7/INDEX.md) | v5/v6中间结果、v7最终回归、暗帧和适用边界 |

冻结v1–v7 YAML与[candidate_detector_v2.yaml](candidate_detector_v2.yaml)保留原位、原字节，以保持模型相对路径和既有命令有效。日志集中在各批logs/；归档与manifest同批存放，文件名及原始数据不重写。
