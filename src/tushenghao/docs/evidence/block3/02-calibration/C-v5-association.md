# C-v2 关联与assignment原始预算测量（待审）

样例720；定位及assignment结构失败记录0，所有失败保留。

规则：原图epsilon1/trim2，assignment epsilon上限3工作px，固定五比例1/3..1；采样步长1工作px。
原图关联指标不应用待求关联上限；定位弧指标仍受已公开recipe划分，须连同规则审批。
同一分层公式见summarize.py；相对面积步长0.005为此次显式候选规则。
采样边界距连续上界可再加step/2；该裕量不隐藏在统计均值中。

| 字段 | 统计候选（待审） |
|---|---:|
| edge_position_original_px | 2 |
| component_mapping_original_px | 4 |
| turn_connection_length_original_px | 8 |
| min_support_span_original_px | 18 |
| producer_edge_position_original_px | 9 |
| producer_edge_direction_deg | 3 |
| assignment_boundary_work_px | 3 |
| assignment_direction_deg | 30 |
| assignment_relative_area_error | 0.045 |

结构失败：
- 无

原始记录 `build/block3-C-measure-v5.jsonl`；逐层统计 `C-v5-association.csv`。
候选不能因成功率自动获批；H/视频不得反向参与定值。
