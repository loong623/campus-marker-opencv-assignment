# Path A 定点修复验收

状态：**Path A 实施与验收通过，用户132帧人工复核全部通过**。Release/Debug完整1676及专项核查、逐帧公共结果比较均PASS。仅本轮Path A完成，不表示整体P0消失；初次受阻与用户批准后的真实过程均保留。

## 完成内容

三L仿射只使用自身通过原explicitL结构规则的拓扑候选唯一实测凹角。不同合法坐标全枚举，同点全部来源保留；生成父追加三条round-trip来源证据，不改统一schema。
原裸凹点/anchor回填删除；组合、原仿射拟合、父验证、M/S补全、角点预算、decode的合法竞争拒绝不改。

| 检查 | 原基线 | Release正式结果 | Debug |
|---|---:|---:|---|
| CTest | 两种23/23 | 25/25 | 25/25 |
| 新专项 | 无 | A01–A13全过 | A01–A13全过 |
| 全视频帧数 | 1676 | 1676（完整性PASS） | 1676（完整性PASS） |
| DETECTED / NOT_DETECTED | 864 / 812 | 976 / 700 | 与Release一致 |
| 旧Path A集合 | 118 | 恢复112、残留6 | 与Release一致 |
| 原864成功退化 | 对照基准 | 0 | 与Release一致 |
| A外raw status变化 | 对照基准 | 0 | 与Release一致 |
| 全部raw变化 | 对照基准 | 132帧：112新增成功+20旧成功四点变化 | 与Release一致 |
| 旧成功方向变化 | 对照基准 | 0 | 与Release一致 |
| 旧成功最大对应屏幕点位移 | — | 1.205078125原图px（不是容差） | 与Release一致 |
| track/display历史变化 | 对照基准 | 204帧（与raw分开） | 与Release一致 |
| 阶段父／锚点来源与affine精确复算 | 原件封存 | 5627父／16881锚点，全PASS | 与Release一致 |
| 当前角点证据validator | 原件封存 | 978测量全PASS | 与Release一致 |
| 语义及float四角/方向发布复算 | 原件封存 | 976检测全PASS | 与Release一致 |
| 新Release/Debug公共逐帧结果 | — | 1676逐帧零差异 | PASS，同指纹 |
| 全132复核帧 | — | 图像/原值JSON/旧数值索引已归档 | 用户132/132 review PASS |

流程判断：原23+新增2工程测试、完整双1676、锚点来源/原仿射精确复算、当前四角/方向生产核查、公共结果一致性均PASS，用户132/132 review通过。
效果判断：Release复现参考112恢复与976成功，原864零新增失检、A外状态不变；20个旧成功raw四点确实改变，必须人工复核。
整体P0判断：并未整体解决。700帧未检测不能直接称700漏检；无独立真值/VQ不报召回或准确率。

新指纹0d2d2e63aef753bc，原指纹c1785fb03a54ed25永久保留。指纹含公开raw/track/display，恢复与历史变化会改变新指纹，不能把差异自动叫回归，也不能覆盖旧金样。
模型锚点复算投影误差最大0work px，只表明这些输入点与原拟合一致，**不是真值误差0**。
Release实测process_total均值78.909ms、p95 205.270ms，运行墙钟135.287s；Debug均值1150.827ms、p95 3033.708ms，运行墙钟1932.013s。两者同机并行，不作性能优化/硬实时达标结论；本轮不改搜索预算。

## 验证方法

[施工日志](path_a_fix_log.md)记录完整Step0–7及批准，全部实录命令、退出码、时长和stdout/stderr见[命令索引](evidence/final-fixes/path-a/commands.md)。
[Release正式25项](evidence/final-fixes/path-a/tests/release/ctest-formal.stdout)、[Debug正式25项](evidence/final-fixes/path-a/tests/debug/ctest-formal.stdout)均退出0，原23个目标全部保留。
[Release完整性](evidence/final-fixes/path-a/check-release.json)、[专项机器报告](evidence/final-fixes/path-a/report-release/path_a_report.json)均PASS，原图/工作尺寸/源时间戳相等，生产配置与原verification相等；输入/模型指纹与真实文件复核。

A01合法L；A02非法点数/短M/NaNInf/索引/epsilon；A03裸凹点第一；A04无来源不回填；A05第二合法锚点；A06同点完整来源；A07非连续ID重排等价父集合；A08精确K含非法affine；A09真实圆角/噪声L与短M；A10提取/观察/生成/验证/补全/decode实际链路；A11合法三L失败父仍unresolved empty；A12截断/同几何异方向/几何冲突/残差选择；A13公共入口同像素/reset/平滑开关raw一致，empty无历史框。

[13项CLI负例](evidence/final-fixes/path-a/step4/negative-cases-attempt-03/results.json)全部命中预期非零：空双方、缺帧、重复帧、无详情、伪造support、改预算、旧成功变空、A外状态改变、已有报告/渲染目录、未知/重复/缺值参数。旧成功与A外两例同时核实际报告损失/范围变化计数，不以任意非零冒充覆盖；受控单帧fixture不是正式视频运行。

代表2/1045当前四点、物理ID、原支持弧、屏幕顺序和方向[0,1,2,3]已由原生产validator/语义/发布核验；图像抽查仅是辅助，不代表用户真值批准。[代表原值报告](evidence/final-fixes/path-a/step4/representative-review.json)、[代表图索引](evidence/final-fixes/path-a/representative-frames-attempt-02/review_index.csv)。
1039(P2)、1500(P1)、1573(P1)仍NO_VALID_ADJACENT_EDGES，未顺带改预算。

正式差异：[118帧分类](evidence/final-fixes/path-a/report-release/path_a_frames.csv)、[raw全字段](evidence/final-fixes/path-a/report-release/raw_changes.csv)、[track/display](evidence/final-fixes/path-a/report-release/history_changes.csv)、[6残留](evidence/final-fixes/path-a/report-release/remaining_failures.csv)。
全部新增成功与旧成功几何变化：[132帧review索引](evidence/final-fixes/path-a/frames/review_index.csv)。每帧原PNG/证据PNG/原值JSON，测量独立图层、注释在目标外，20变化帧链接旧记录和旧数值、橙色旧框只作对照。readback核264PNG可读、132原图与视频像素严格一致：[PNG核查](evidence/final-fixes/path-a/step6/png-frames.json)。

## 踩坑记录

初次manual_validation_check因无拓扑且等长内臂mock失败。终稿原白名单遗漏该测试，先如实受阻；用户随后明确批准相关fixture修改。现在三个模型/观察/组件同步为合法长臂L，并由原observeLTopologies取得候选，原“生成→验证至少一个父”的用例目的保留；未删断言、未恢复fallback或改生产门限。
[首次失败Release日志](evidence/final-fixes/path-a/tests/release/ctest-final-blocked.stdout)、[用户批准后25/25](evidence/final-fixes/path-a/tests/release/ctest-approved-fixture.stdout)与[实际fixture补丁](evidence/final-fixes/path-a/failures/manual-fixture-proposal.patch)都保留。
原geometry_l_topology手造两点/一点伪候选改为完整六边候选，保留全锚点/精确预算目的，另查裸凹点不得借类名准入；内部多候选mock不声称来自同一真实轮廓，真实观察由A09/A10及原用例覆盖。

新A07首版签名误含assignment向量顺序，改为按part/source/anchor排序，并核每父拟合；A11首版额外组件工作尺度错，修正后确实到必要邻边取证失败。verifier首版错误用OpenCV专属YAML读取普通manifest，改为严格扁平schema解析。伪造support首版改少了来源，先被完整数量检查拒绝，后只改下标确认独立索引核查。全部失败命令/新尝试目录保留。

## 已知限制

剩余153(P2)、280(P3)、648(P1)、1108(P0)、1573(P1)各1合法父/0测量，必要角邻边取证失败；382有2合法父/1测量，失败父P2仍无法排除。382的两个L3锚点(583,328)/(586,331)各有合法拓扑来源，不能因另一父成功跳过它。
[逐帧阶段分析](evidence/final-fixes/path-a/step6/remaining_analysis.json)、[6失败图/测量说明](evidence/final-fixes/path-a/remaining-frames/review_index.csv)。失败图中的measurement只是已有取证，不是成功Detection；382当前raw仍空。

人工关卡：用户明确确认112新增成功+20几何变化帧共132条全部复核通过。[审批原文与索引SHA](evidence/final-fixes/path-a/step6/user_review_approval.json)、[审批后索引](evidence/final-fixes/path-a/step6/review-approved.csv)。机器导出的PENDING表示生成时状态，原CSV保留，不覆盖原证据；当前人工gate为PASS。不据此推导700空帧召回或量化真值误差。
PathB、564基线无几何假设尚未实现／延期fix2；24未完成M/S不修。仍可能闪烁，不做性能优化/调参，不保证一般透视、任意距离/光照或硬实时。最大1.205078125不是生产容差。
两种构建完整性与专项均PASS；[Debug完整性](evidence/final-fixes/path-a/check-debug.json)、[Debug专项报告](evidence/final-fixes/path-a/report-debug/path_a_report.json)、[Release/Debug逐帧比较](evidence/final-fixes/path-a/release-debug-compare.json)。两个新公共指纹均0d2d2e63aef753bc，1676帧failure_indices/minimal_differences为空。
归档链接/保护hash/固定build清理结果见[施工日志](path_a_fix_log.md)和[最终哈希清单](evidence/final-fixes/path-a/hashes.json)。

归档清理：[保护前置核查](evidence/final-fixes/path-a/step7/prepare_archive.json)、[精确清理记录](evidence/final-fixes/path-a/step7/cleanup.json)。本轮两固定build已删除，原33个build子目录含editor保留；无需build唯一证据，README命令可重新构建。原用户run、历史证据、配置及模型无变动，原13项历史前缀也经SHA验证保留。最终文档链接与完整哈希清单在归档末尾封存。
