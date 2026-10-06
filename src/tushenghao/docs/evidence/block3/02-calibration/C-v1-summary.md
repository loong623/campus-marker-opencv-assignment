# Block3 C实验统计（recipe v1；尚非批准预算）

总样例 720，有效 720，结构失败 0；预期正式C为720。

固定recipe含未批准的简化/位置/圆角剔除/连接弧/跨度/病态参数；只报告实验事实，不能写回生产配置。
任何结构失败未解决前，成功样本的统计候选均不可批准。样本不全也不能称正式C完成。

方法：样例内最坏值；样本标准差n−1；nearest-rank P99/P01；按工作尺度/圆角/噪声/L-M分层。
上限max(μ+3s,P99)，像素向上0.5px、角度向上1°；点数min(μ−3s,P01)，向下整数且≥3。

| 字段 | 实验候选（非批准值） |
|---|---:|
| max_corner_error | 1.5 |
| max_edge_direction_diff | 1 |
| max_support_extension | 9.5 |
| max_line_fit_residual | 0.5 |
| min_edge_points | 10 |
| truth_error_budget | 2 |

逐层n/失败数/μ/s/分位数/最大值/最坏case见 `C-v1-summary.csv`。
原始证据：`build/block3-C-measure-v1.jsonl`。

失败明细：
- 无（仅说明本recipe及已测样例）。

用户批准记录：待审。H/全视频/V/N/Q：NOT_RUN/未确认。

后续审批：本会话2026-10-05用户批准六项统计候选，其余仍待审；见[预算审批表](../../../block3_budget.md)。本页原始实验状态保留。
