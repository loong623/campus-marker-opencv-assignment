# Block5 可观测性实施与验收

## 完成内容

实现已落地；验收状态见下表。全量Debug保留一项原基线失败，不宣称全部检查全绿；最终review与Git由用户处理。
依据[批准终稿](ref/Block5_可观测性_Codex完整实现方案_终稿.md)，第0步核对分支`feat/observability`与起点`b9cccd4a86ac5959a8c67559cde1d399ba29e046`，源树无先行代码差异，5个旧CSV仅CRLF/LF解释；旧归档70项hash一致。

Route B从物理P0–P3直接转float再共享规范排序，检查表示合法性，一次提交全部Detection，失败保留原measurement并整帧拒绝。P01–P06通过，当前视频无实际置换。D02无分配reset快照保留三种原因/次数/最后原因，外部来源null，避免Detector/Temporal双计，非法稳定输入诊断在清历史前复制并完整序列化。

六个lib模块内增加实例FrameRecord通道、九阶段状态/计时、唯一serializer/recorder/report；三audit共享装配，App独立baseline/debug与严格配置全链。公共布局/四调用及vector<string> diagnostics不变，原模型/几何/预处理/取证/语义数值锁定；r=2px、偏离=2px不变。RAW/STABLE仅当前，empty无旧有效图形，unknown不回填，HISTORY只文字，renderer克隆源图。离线不伪造实时延迟，视频导出开启明确报NOT_IMPLEMENTED。

当前实际生产代码摘要`755942ea2758e88e6413ed595b92e7865a61e5a38f938d95f9776b0ae76cf3c0`，外部提交标签只表示起点；[最终源清单](evidence/block5/environment/final_source_manifest.json)与原始/补丁前置清单分别保留。输入SHA256为`aa1219a7a7b702ea1265be8752853a267c982a651afa0f7846f516f9517c9ac7`，1440×1080、1676帧、fps70.408336；实际尺寸逐帧核验，见[输入身份](evidence/block5/environment/video_identity.json)。

## 验证方法

全部命令、起止、真实退出码、失败现场与相对路径见[证据导航](evidence/block5/INDEX.md)和[命令记录](evidence/block5/archive/commands.json)。未运行检查不填PASS。

|检查|实际结果|证据|
|---|---|---|
|原始基线Release / 视频|19/19；1676帧与旧Block4零差异|step0/original_archive_comparison.json|
|Route B|Release19/19；相关Debug5/5；P01–P06；视频0差异|step0/route_b_comparison.json；comparison/route-b-cpp-check.json|
|最终Release|23/23|tests/step7-evidence-release-ctest-05.log|
|最终相关Debug|9/9|tests/step7-final-debug-related-04.log|
|最终全量Debug|22/23；唯一旧geometry_matcher_test断言失败，原基线同样SIGABRT|tests/step7-evidence-debug-full-03.log；comparison/legacy_debug_failure.json|
|M01–M08 / R01–R09 / V01–V07 / I01–I10|34组主动断言通过，覆盖异常/重置/有限数/引用/采样/图层/配置|tests/step7-final-active-02.log|
|旧Block4主动检查|30组保留并通过；旧Block3检查及新增发布检查保留|tests/step7-final-active-02.log|
|H720|720 stage、720 isolated、wrong_valid0；旧金样有效结果0差异|comparison/H-final-comparison.json；comparison/H-final-regression.jsonl|
|正式baseline|全1676、selected0、864 detected/812 empty、无非法/NOT_READY；首帧计入|runs/app-baseline-03/summary.yaml|
|debug验证用途全帧|独立公共process；mode debug / purpose verification；1676详情、图层离屏开启|runs/app-debug-03/|
|补丁金样与最终debug|公开结果/共同原始证据/准入原因/截断/稳定逐帧0差异|comparison/patched-debug-03.json|
|结果指纹连接|patched、baseline、debug、temporal一致`c1785fb03a54ed25`，不代替逐帧比较|summary及compare报告|
|三audit all|各1676，固定九槽、scope真实，process_total=NOT_EXECUTED；decode raw及temporal stable共同结果exact|comparison/*all-01-check.json；patched-decode-all.json；patched-temporal-all.json|
|原网格|480×32=15360；原结果/temporal/真值逐字段exact|comparison/grid-comparison.json|
|输出约束|empty无track、unknown无回填；最大当前偏离1.994915px≤2；confidence/code null|comparison/video-invariants.json|
|真实错误回归|19类CLI错误按预期非零；另真实零帧EOF失败留FAILED，无成功summary|comparison/negative-cases.json；zero-frame-check.json|
|归档与清理|以archive最终核查/清理收据为准；失败证据和原基线二进制保留|archive/中的final-archive-check系列|

正式性能使用baseline公共调用边界，Release/GCC13.3/OpenCV4.6/WSL2/i9-14900HX、OpenCV实际32线程，未改线程参数。N1676、首帧process_total=46.776328ms仍在全量，吞吐10.536088fps；14ms为用户系统目标，**未达到**，不据此优化算法。

|阶段|N|mean ms|median ms|p95 ms|p99 ms|max ms|
|---|---:|---:|---:|---:|---:|---:|
|capture|1676|0.935255|0.904482|1.218541|1.567732|45.687847|
|preprocess|1676|0.433899|0.421059|0.556113|0.629075|6.952689|
|detect|1676|0.871722|0.896076|1.468737|4.065235|11.588475|
|decode|1676|92.645987|84.327170|271.428089|373.832948|906.796375|
|stabilize|1676|0.004928|0.004737|0.008022|0.018538|0.277855|
|process_total|1676|93.969492|85.834156|273.198040|375.208491|908.780922|
|visualize|0|N/A|N/A|N/A|N/A|N/A|
|wait|0|N/A|N/A|N/A|N/A|N/A|
|export|0|N/A|N/A|N/A|N/A|N/A|

baseline实际诊断构造区间均值4.495695us、框架差额均值12.955262us。构造区间仅begin/event/output，详情复制已计入真实阶段与total；不能称所有日志成本。debug公共调用均值92.469699ms，visualize均值0.459480ms，export均值0.263231ms；wait未执行。baseline/debug差值不解释为诊断精确开销。summary结束写盘与侧表补写见各run的run_export.yaml；capture/GUI/编码/等待/写盘均在公共process之外。

持续执行起点记录为12:10:44（北京时间），本表生成于2026-10-06T13:18:54.756876+08:00；命令墙钟与主动编写未分开计量，不能补造纯编写秒数。下表给实际可追溯的步骤首尾和命令耗时合计（含并行与嵌套，不等于整段主动时长）；初始四条原始基线命令无独立时间，明确null。后续归档结束时点见最终收据。

|步骤|实际命令首尾（北京时间）|记录命令墙钟秒合计|状态/修复/产物|
|---|---|---:|---|
|0|2026-10-06T12:13:51.889605+08:00 → 2026-10-06T12:18:05.246319+08:00|203.448|原始基线/Route B；P01–P06通过；step0/|
|1|2026-10-06T12:19:56.606434+08:00 → 2026-10-06T12:22:20.445990+08:00|19.706|内部通道/重置；修正fixture原因判断；tests/|
|2|2026-10-06T12:22:36.745588+08:00 → 2026-10-06T12:23:59.896080+08:00|9.537|阶段计时与配置装配；tests/|
|3|2026-10-06T12:26:10.026443+08:00 → 2026-10-06T12:26:14.133217+08:00|4.107|统计/统一记录；tests/|
|4|2026-10-06T12:29:47.567956+08:00 → 2026-10-06T12:33:18.682787+08:00|21.698|App与三audit/verify；补齐声明头；tests/|
|5|2026-10-06T12:34:14.841176+08:00 → 2026-10-06T12:34:49.499772+08:00|5.917|四个主动测试；补齐verify声明头；tests/|
|6|2026-10-06T12:35:31.900686+08:00 → 2026-10-06T12:39:58.976365+08:00|40.570|Release23/23、相关Debug；完整Debug既有失败保留；tests/|
|7|2026-10-06T12:39:48.107416+08:00 → 2026-10-06T13:18:39.079444+08:00|1792.377|全视频/三audit/网格/H/失败与补强；runs/ comparison/|
|8|2026-10-06T13:11:50.390842+08:00 → 2026-10-06T13:13:38.108117+08:00|9.842|输入/来源/重放/链接与归档；archive/|


## 踩坑记录

- 阅读约束前误用两次只读Git查询；未修改Git。随后直接读HEAD/ref/index/对象；原始及最终CMake无Git执行。此偏差保留，不冒称从未运行Git。
- 初次读packed子树失败，同shell继续configure，未先写标识；为本会话自建目录补标识，失败日志保留，后续独立subprocess按退出码推进。
- 初步reset测试把非reset事件也算reset，导致I03失败；修正测试为枚举判断后实例隔离通过，未删断言换绿。
- runner/verify缺声明头导致三次编译失败，日志保留，补include后通过。
- 全量Debug的旧三L fixture不满足当前旧模型/观测契约；未改测试/几何算法，原基线库+原测试及归档二进制均复现同一assert。
- 旧H工具首次退出0但父目录缺失，未写产物；该次不算通过，保存缺失记录，建目录后重跑并核720条。最终版本再次复跑H。
- 首轮baseline有效配置中模型相对路径重载错误，早期verify未核模型存在而误报PASS；保存问题记录，runner规范解析后的绝对路径、verify核实际模型文件与指纹，正式采用03运行。01/02仅过程证据。
- 网格首次比较未适配旧result.diagnostics固定为空的布局；新网格仅镜像原temporal两项原因，适配器逐项核这些字符串等于旧载荷，并严格比较完整temporal及其它结果。原首轮差异报告/日志保留，未放宽几何或原因比较。
- 复查补齐最后reset原因、直接稳定非法raw详情与原角点error_，增加真实非法update/恢复和序列化断言；重新构建、测试及完整视频，不沿用修复前运行冒称最终结果。

## 已知限制

旧geometry_matcher_test完整Debug仍失败，用户review需明确接受既有失败或另开授权修fixture；本轮保留此项，未宣称完整Debug23/23。新主动组M/R/V/I均已跑，但M04用受控throw作用域及失败recorder验证异常状态，没有人为给Detector算法注入故障钩子。
14ms未达；视频无人工V/Q标签，不报告准确率。D23三L面积规则、D25硬绑定、一般透视/远距离/任意光照、持久身份、置信度/多目标保持旧范围。GUI本环境无显示，验证显式请求失败路径；离屏绘制已测，未运行可见窗口的人工交互退出。
少数早期增量失败日志没有独立源快照，manifest明确标intermediate/unavailable，不能用最终hash替代；正式验收运行均有最终源摘要和可核对指纹。剩余动作是用户review、既有Debug失败决策与用户Git操作；无新的数值审批请求。
