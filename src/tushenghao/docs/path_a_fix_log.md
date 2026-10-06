# Path A 施工日志

状态：Path A实施与验收通过，用户132帧review全PASS。原Step3阻塞获用户批准并解决；两构建25/25、完整双1676、专项与跨构建公共结果检查通过。归档、文档链接与哈希封存完成，两个固定build已清理，本轮收尾完成。只修三 L 锚点资格；Path B 与无假设 564 帧尚未实现／延期到 fix2。
不修改角点、预算、decode 竞争拒绝、公开接口或原证据，不执行 Git。

## 基线与决定

现场分支 feat/final-fixes，HEAD 189790850cf8960ad19a023092f274e8468c39c4；文档审查快照 e0b9d185d66440abe70aaebd885012ad1ed0a946。
源码摘要与用户 new-runs/verification 完全一致：560122f43e2fc8c39e85ea3e2998f950160871b0b2cb8d50bb968d245e3a0ed9。
旧运行 1676 帧、864/812、c1785fb03a54ed25。118 帧集合直接取旧 details.decode.diagnostics 的竞争未解标识。
输入／配置／模型与源码保护 hash、旧 118 ID 和六份原件复制比对见 [修改前清单](evidence/final-fixes/path-a/step0/prechange.json)。
HEAD 与审查快照不同，但目标代码与用户运行源码一致，无目标改动冲突；以现场只读基线推进，不替换旧运行。

## Step 0–7 实际执行

| 步骤 | 开始／结束及实测耗时 | 操作、产出、命令退出码 | 状态／剩余 |
|---|---|---|---|
| Step0 | 见修改前清单 started_utc，已完成 | 核分支/源码、复制六份基线、提取旧 118；[命令索引](evidence/final-fixes/path-a/commands.md) | PASS：原 Release/Debug 各 23/23，视频实解码1676、旧运行完整性 PASS |

## 根因与实施决定

L 类别只是候选剪枝，不能把所有 raw CONCAVE 赋予 L 凹角资格。复用 explicitL 对每个拓扑候选核验，
保留不同实测锚点及同点全部来源；不从 turns 或 anchor_vertex_index 回填。合法父取证失败仍保守 unresolved，decode 不改。

## 排障与后续

当前尚无本轮失败结论。112 恢复／976 成功为隔离实验参考，不作为调参目标；新输出和旧 raw 变化都须记录证据并交 review。

Step1：先写 A01–A09。helper-only 在 matcher 接入前只跑收集／结构／真实观察子例，A04/A05/A07/A08 的生成器断言留 Step2 全量执行，未削弱最终测试。

Step1 已实现 wrapper/helper；Step2 接入逐候选锚点和三条来源证据。原 affine 拟合／组合顺序／预算／decode 均未改，不增加新诊断计数。

排障：Step2 A07 首版把 assignment 向量顺序放进签名，容器反转后合法映射同集却签名不同；改为排序后的 part/component/anchor 来源集合，并逐父核拟合落于实测点。Step3 首次构建新测试误用 ScreenOrderResult 字段名，已改为真实 screen_order_；失败日志保留。

Step3 硬关卡受阻：原 23 项中 manual_validation_check 由 PASS 变 FAIL。其 createLObservation 没有 l_topology_candidates_，只给类名、turns、anchor；20×20 且 10 内臂/10 笔画 mock 的 explicitL 比值为1，也不符合原长臂规则。严格删除回填后零父是正确准入结果。该测试不在白名单，未修改、未删除或隐藏；不恢复回填、不改 explicitL 规则。按终稿停在工程关卡，不进入代表帧/全视频/正式 verifier 阶段。
A11 首版新增矩形由960×720预处理却附在1440×1080工作图，导致 MISSING_ORIGINAL_SUPPORT；观察到这是测试侧坐标错误，修为与 scene 工作图同尺寸，以便确实到原图边取证失败。生产文件未改。

## 首次受阻时间点（用户批准前的历史记录）

下表仅使用 commands.json 实际命令始末和实测耗时；并行命令合计不等于墙钟，编辑未独立计时。
修改前清单记录 Step0 起点，所有命令完整 argv／退出码／stdout／stderr 在 [命令索引](evidence/final-fixes/path-a/commands.md)，没有将未运行步骤记为成功。

| 步骤 | 实录验证起止 UTC／耗时 | 结果 |
|---|---|---|
| Step0 | 2026-10-06T11:29:35.816637+00:00 → 2026-10-06T11:33:13.662080+00:00；命令耗时合计 61.089s | PASS：源码与用户旧运行一致；原23/23×2，完整旧1676，六份副本 SHA 相同。 |
| Step1 | 2026-10-06T11:34:25.392364+00:00 → 2026-10-06T11:34:51.923527+00:00；命令耗时合计 2.932s | PASS：先写A01–A09后实现；helper-only 五项通过。 |
| Step2 | 2026-10-06T11:34:52.073355+00:00 → 2026-10-06T11:36:02.106229+00:00；命令耗时合计 1.597s | PASS（最终A01–A09）；首版A07失败已保留，原因是签名误含 assignment 顺序。 |
| Step3 | 2026-10-06T11:36:02.347099+00:00 → 2026-10-06T11:38:31.881371+00:00；命令耗时合计 40.346s | FAIL：最终 Release/Debug24/25；A01–A13全通过，manual_validation_check原件未改。 |
| Step6（受阻证据） | 2026-10-06T11:38:23.893537+00:00 → 2026-10-06T11:40:33.940260+00:00；命令耗时合计 1.914s | PASS：未应用的额外fixture建议在临时副本通过，不计CTest过关。 |
| Step4 | NOT_RUN | 硬关卡未通过；代表帧、正式 path_a_verify／render 与负例未执行／未实现。 |
| Step5 | NOT_RUN | 未跑新版本双构建全1676，不声称112恢复或976成功。 |
| Step7 | 归档核查中；结果随后记录 | 交付受阻验收、三文档同步、保护hash、仅清本轮固定两build。 |

### 修改清单与 fixture 判断

生产：geometry_l_topology.hpp/.cpp 新wrapper复用explicitL，geometry_anchor_evidence.hpp/.cpp新收集；geometry_matcher.cpp只改准入／来源。
CMake增加1库源码、2CTest（原23名字保留）；tests/geometry_l_topology_test.cpp修合法完整六点fixture，保留第二锚点、48/47精确截断、validator传播、来源ID和真实观察旧子例。
新tests/geometry_anchor_evidence_test.cpp A01–A09，tests/path_a_competition_test.cpp A10–A13。公开接口、decode拒绝、配置、角点、补全、原证据无改动。
新增本文和path_a_acceptance.md，README/INDEX/final-fixes_acceptance只追加本轮状态；正式专项工具因Step3受阻未创建。

原 anchors() 的两个完整长臂候选只检内部收集/枚举，不声称同轮廓真实共存；A09/旧圆角与噪声轮廓/A10真实链路覆盖来源真实性。
originalAnchors()保留入口，改查 unsupported raw被排除且合法good保留，不通过恢复错误fallback换绿。
A11增加当前实测白色矩形的独立M对应，三个L锚点／合法source引用／affine不变；它确实到NO_VALID_ADJACENT_EDGES:P1才失败，成功父与该父并存仍UNRESOLVED empty。

### 最先失败关卡与具体建议

[额外fixture补丁（未应用）](evidence/final-fixes/path-a/failures/manual-fixture-proposal.patch)及[对照源码](evidence/final-fixes/path-a/failures/manual-fixture-proposed.cpp)把manual mock改为32×32、8px笔画的长臂L，所有model/observation/component/area同步，并由原observeLTopologies取得来源。
保留原恒等对应、生成→父验证正例断言；不修改production门限。两次[临时对照验证](evidence/final-fixes/path-a/failures/proposal-check-archived.stdout)均退出0。
这不是白名单内正式测试通过：原 manual_validation_check 始终保留且 FAIL，需要用户明确批准此额外测试文件后才能继续Step4–7完整验收。

### 后续判断（不在本轮追加实施）

1. **已验证的计划遗漏**：manual_validation_check::createLObservation 缺来源且短臂mock不合准入。最小实验为归档补丁对照（已PASS）；下一步仅批准该fixture修改，不改算法。
2. **假设**：参考实验残留382的合法父可能仍有角点证据竞争；定位decodeStage/resolveObservedCorners。最小实验是工程过关后按统一日志核每个合法父的原图支持弧及失败原因，若属PathB预算问题延期fix2，不按NO_VALID_ADJACENT_EDGES豁免。
3. **待验证**：参考20帧旧成功raw变化可能来自去伪父后的合法测量选择，不能凭1.205078px认定正确。定位resolveSemantics与统一CornerEvidence；最小实验为正式verifier逐字段比较、全部变化帧证据图及用户review。本轮未跑，不能将实验表冒充当前结果。

P0整体闪烁未解决；PathB、基线564无假设、24未补全M/S尚未实现／延期fix2；性能只记录已执行构建/测试，无视频性能新结论。

## 用户批准后继续

2026-10-06T11:40:52.473904+00:00：用户明确批准修改manual_validation_check相关测试文件，要求同geometry_l_topology_test修正fixture并保持目的。已核原件SHA无外部变化，应用归档对照补丁，不改生产规则；将重新跑Release/Debug25项。上述受阻记录为此前真实状态，未删除历史。

用户批准后的Step3：两个原构建分别重编manual测试；正式CTest Release/Debug均25/25，A01–A13全过。恢复Step4，代表帧真实原视频decode_audit显式子集2/1039/1045/1500/1573；该子集不冒充全视频。

Step4 verifier首轮：manifest为普通YAML且环境键有引号，OpenCV FileStorage专属解析器拒绝。改专项工具按原observability_verify扁平schema严格解析manifest，生产记录不改。首轮代表帧render和CLI负例失败日志保留，新尝试用新目录。

Step4 CLI attempt02：空/缺帧/重复/无详情/改预算/旧成功变空/A外状态变化/目录保护/未知重复缺值均正确非零。伪造source首版同时改少了来源数量，被更前的incomplete topology supports拦下（安全拒绝正确，但预期原因不匹配）；attempt03只替一个下标，保留完整数量以独立覆盖索引检查。

Step4完成：代表原图2/1045得到当前合法四角，方向[0,1,2,3]；render实际从0解码1676，为五个代表帧导出原图/独立测量图/原值JSON。1039阻断P2邻边、1500与1573阻断P1邻边，不修改PathB预算。正式CLI13项负例attempt03全部正确非零且命中目标（旧成功变空/A外变化还核实际报告计数为1）。保护清单2162件核查通过，原用户文件与禁改生产没有越界。准备正式双1676。

Step5 Release完整回归及专项工具PASS：1676，976/700，指纹0d2d2e63aef753bc；原864零退化，旧A118恢复112、残留6，A外状态零变化；旧成功raw角点变化20、方向0，最大1.205078125原图px，历史变化204。5627阶段父/16881锚点来源及affine精确复算、978当前measurements原validator、976公开发布均PASS；模型锚点投影误差0不是真值误差0。仅382仍unresolved。Debug尚在进行，不冒充双构建已完成。
Step6开始导出全132 review帧，来源为统一测量数组，用户review pending。

## 当前后续判断（Release实证，Debug待齐）

1. **实证**：原噪声凹点借L类别生成伪父是直接根因；112恢复与零旧成功退化支持定点修正，不支持放宽角点预算或删合法失败父。资格不变成“选最高分”；全部合法锚点仍枚举。
2. **假设，fix2**：153/P2、280/P3、648/P1、1108/P0、1573/P1的NO_VALID_ADJACENT_EDGES可能由真实支持弧长度、连接、方向/位置或残差条件共同阻断。文件corner_edge_fit.cpp::fitObservedEdgePair。最小实验：测试侧读取这五个原图/已合法父，逐候选记录第一拒绝门槛并与支持弧匹配；不能由汇总计数认定应放宽哪个预算，本轮不改PathB。
3. **实证＋假设，fix2**：382的L3来源分别(583,328) supports0:4和(586,331) supports1:4，两个都合法；第二父P2失败、第一父成功测量一份，现冻结规则继续empty。最小实验：测试侧独立审阅两父当前拓扑与P2邻边条件，确认是否存在可被原证据反证的解释；未证明前不能跳失败父，也不在本轮引入失败豁免状态机。
4. **待人工review**：20个原成功四点改变，方向0变化，最大1.205078125px；这不是容差或真值结论。resolveSemantics保留测量可能因去伪父而变化。最小实验：在132帧索引中先看20变化帧的旧数值/当前四角弧/方向来源，本轮已导出，用户批准尚未收到。
5. **范围限制/假设，fix2**：564基线无几何假设未修，不能默认真空或漏检；geometry_observation/generateGeometryHypotheses仅定位阻断阶段。最小实验是对原图独立盲标必要结构与亮度/成像条件，再比观察记录；不看新检测结果反向选择可检样本，不改本轮门限。

所有剩余六帧的当前阶段、父数、测量数和原因见[remaining_analysis](evidence/final-fixes/path-a/step6/remaining_analysis.json)与[失败证据索引](evidence/final-fixes/path-a/remaining-frames/review_index.csv)。
132个新成功/变化帧[review索引](evidence/final-fixes/path-a/frames/review_index.csv)全部PENDING，机器PASS不取代人工真值与方向review。

2026-10-06T13:03:49.228843+00:00：用户明确确认“132帧人工复核全过，无问题”，并要求按流程跑完Debug、写文档、清本轮build收尾。人工gate已PASS，不重复审批；[确认记录](evidence/final-fixes/path-a/step6/user_review_approval.json)绑定原132索引SHA与帧ID，[确认后的索引](evidence/final-fixes/path-a/step6/review-approved.csv)。机器导出时PENDING原件保留为历史状态，未覆写。此批准不代表整体P0或未标注700空帧已解决。

Step5 Debug结束：完整1676，976/700，新指纹0d2d2e63aef753bc；完整性/专项均PASS，指标与Release逐项相同。原observability_verify跨构建逐帧公共结果PASS，failure_indices与minimal_differences均空。Debug实测运行1932.013s（不是实时fps），不优化预算。
Step6全部完成：132审核图用户全通过，另存6残留图与5代表图；PNG读回可读286张，143份原图全部与视频像素严格一致。导出时的PENDING原件保持，用户后批已单独关联。Step7开始最终保护/归档/hash/链接核查，再清仅两个有标记固定build。

## Step0–7 最终实录（取代此前历史时点状态）

时间来自实际commands.json的验证命令开始/结束，耗时是该步命令实测合计；并行总和不等于墙钟，代码编辑未独立计时。所有具体argv/返回码/输出均在永久命令索引；失败尝试计入而未抹掉。

| 步骤 | 实录验证起止UTC | 实测命令耗时合计 | 最终结论 |
|---|---|---:|---|
| Step0 | 2026-10-06T11:29:35.816637+00:00 → 2026-10-06T11:33:13.662080+00:00 | 61.089s | PASS：原双23/23、实解码1676、旧基线完整、六副本SHA相同 |
| Step1 | 2026-10-06T11:34:25.392364+00:00 → 2026-10-06T11:34:51.923527+00:00 | 2.932s | PASS：wrapper/helper和先写A01–A09 |
| Step2 | 2026-10-06T11:34:52.073355+00:00 → 2026-10-06T11:36:02.106229+00:00 | 1.597s | PASS：matcher准入/来源接入，最终A01–A09通过 |
| Step3 | 2026-10-06T11:36:02.347099+00:00 → 2026-10-06T11:49:55.810344+00:00 | 63.600s | PASS：经用户批准修manual fixture后双25/25；初次失败保留 |
| Step4 | 2026-10-06T11:41:10.761947+00:00 → 2026-10-06T11:48:42.690311+00:00 | 26.162s | PASS：五代表帧证据，tool13负例命中，源/配置保护 |
| Step5 | 2026-10-06T11:46:07.499573+00:00 → 2026-10-06T13:04:06.302293+00:00 | 2078.659s | PASS：双完整1676、专项、跨构建零公共差异；新976/700 |
| Step6 | 2026-10-06T11:51:57.779105+00:00 → 2026-10-06T11:55:52.457261+00:00 | 21.886s | PASS：132review帧+6残留图；286PNG读回与143原图像素相同；用户132全部通过 |
| Step7 | 2026-10-06T13:07:48.894736+00:00 → 2026-10-06T13:10:25.219990+00:00 | 1.600s | PASS：归档/保护完成，精确清本轮2build，33原子目录保留；最终链接/哈希封存随后执行 |

[清理记录](evidence/final-fixes/path-a/step7/cleanup.json)：先归档CMakeCache/CTest定义/LastTest以及原命令、运行、CSV/JSON、PNG，再核精确绝对路径和.path-a-owned标记，仅删除final-fixes-path-a-release/-debug。33个原build子目录（含editor）保留，用户new-runs及历史证据无改动。本轮已归档临时cpp/程序清除，最终文档链接核查/哈希封存在清理之后完成；没有依赖被删build的唯一证据或永久链接。
[归档保护前置核查](evidence/final-fixes/path-a/step7/prepare_archive.json)确认2153原文件SHA不变，9个旧文件只在授权范围修改；README/INDEX/原13项报告历史前缀SHA完全相同，用户8份运行原件/六副本、视频/配置/模型SHA均不变。

本轮新文件为geometry_anchor_evidence.hpp/.cpp、geometry_anchor_evidence_test.cpp、path_a_competition_test.cpp、path_a_verify.cpp、本文和path_a_acceptance.md；原修改9文件及全部新增证据见最终hashes.json。禁止文件/预算无变动，不执行Git。
原源码560122f43e2fc8c39e85ea3e2998f950160871b0b2cb8d50bb968d245e3a0ed9；新真实98个生产/审计源文件摘要ec589deb31651c9acbd2e4b365ab620e825e7d87252979470dc7345be6e3d259，与两次正式manifest完全一致。源码摘要与公共结果指纹是不同口径，均非提交标签。

最终结论：Path A定点修复、流程与检测效果门槛、132帧用户review均通过，本轮完成；P0整体仍未解决，PathB/564无假设/24未补全M/S不在本轮实施。新成功976不是真值召回率，剩余六帧原因保留，112恢复没有靠阈值放宽或合法失败父豁免取得。

最终链接/归档封存检查PASS；[本地链接报告](evidence/final-fixes/path-a/step7/final-links.json)、[SHA256清单](evidence/final-fixes/path-a/hashes.json)。输入/模型/配置/原件保护、两build已清/33旧目录保留均复核；日志及数据一次最终封存，不再追加未授权修改。
