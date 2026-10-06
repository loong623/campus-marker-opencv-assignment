from pathlib import Path
import json,hashlib,datetime,statistics
root=Path(__file__).resolve().parent;docs=Path('src/tushenghao/docs')
l=json.loads((root/'localization-statistics.json').read_text());grid=json.loads((root/'grid-summary.json').read_text())
def fmt(x):return 'N/A' if x is None else f'{x:.6f}'
budget='''# Block4预算与审批（待批准）

2026-10-06；算法基线08c5c44，实验版本Block4-v1。**两个生产值均未批准，生产YAML省略两个键，公共process无论平滑开关均NOT_READY。** C/H数据充分，当前是APPROVAL_PENDING，不是BUDGET_BLOCKED。方法已批准不等于数值已批准。

## 一页选择摘要

`correspondence_uncertainty_px`候选为 **2.0原图px**。C十二层分别计算`max(均值+3×样本标准差,P99)`，向上到0.5px，取各层最大；最坏层480工作宽、2px圆角、无噪声，公式原值1.600070px，取整2.0px。C原始四角最大误差最大1.414214px；H720次全可测，超2px为0/720，最大1.346291px。没有用H或视频倒调候选。T09/T10/T10b分别验证物理对应、唯一循环和歧义原始回退；网格错对应为0。

`max_smoothing_deviation_px`是用户允许的相对当前测量滞后，不能从定位3σ自动推导。下表为14ms、方向已知、C固定扰动的有限对照；回退含首帧NO_HISTORY=1。单位原图px，静止抖动取32帧逐角均值去中心后的RMS，运动段不计算抖动。

| 偏离实验档（未批准） | 静止raw/stable抖动/比值 | 静止raw/stable真值RMSE | x平移1px/次：平均滞后 / 平滑 / 回退 | x平移4px/次：平均滞后 / 平滑 / 回退 | x平移8px/次：平均滞后 / 平滑 / 回退 |
|---|---|---|---|---|---|
'''
for b in [.5,1,2,4]:
 chosen={r['rate']:r for r in grid if r['dt_ms']==14 and r['known'] and r['noisy'] and r['mode']=='translation' and r['deviation_experimental_px']==b}
 static=chosen[0];columns=[]
 for rate in [1,4,8]:
  r=chosen[rate];columns.append(f"{fmt(r['lag_mean_px'])} / {r['smoothing_calls']} / {r['fallback_calls']}")
 budget+=f"| {b} | {fmt(static['static_raw_jitter'])}/{fmt(static['static_stable_jitter'])}/{fmt(static['jitter_ratio'])} | {fmt(static['raw_truth_rmse'])}/{fmt(static['stable_truth_rmse'])} | "+' | '.join(columns)+' |\n'
budget+='''
可考虑r=2px、偏离=2px作为平滑保留与滞后的折中；这是推荐实验组合，**未替用户选择**。若优先严格贴近当前点，1px保留相同静止抖动收益，但4/8px每调用运动基本全部回退；4px允许更多平滑，却在8px/次运动出现约3.27px平均滞后与更大真值RMSE。没有单一“最优值”，最终两个值、适用范围由用户明确确认。若选择其他档只补该档必要对照，不改算法。

批准后需记录原文、日期、两个值/单位、数据/算法版本、适用范围，同步生产YAML及有效配置并重跑相关主动测试和一次正式稳定视频回归；未批准前不称Block4完成。当前生产全视频只验raw非回归及NOT_READY门控，不能代替批准配置的稳定验收。

## 定位统计

每条记录按物理orientation取对应连续名义角真值，保留四角欧氏误差，统计单样例四角最大值。n−1样本标准差，nearest-rank分位数；失败不作0。C/H各720次，十二层，各层无噪声n=24、有噪声n=96，结构/方向失败均0。原图1440×1080、笔画16px、中心C(720,540)/H(1000.25,750.5)、旋转0～345°每15°、圆角0/2px、灰度σ0/1、C seeds11/22/33/44，H55/66/77/88、工作480×360/960×720/1440×1080。

| 数据集/工作宽/圆角/σ | 总/可测/失败 | 均值 | 样本s | P95/P99/最大 | 公式原值/取整候选 | H超r数量/比例 | 支持缺口 |
|---|---|---|---|---|---|---|---|
'''
for r in l['statistics']:
 budget+=f"| {r['dataset']}/{r['work_width']}/{r['radius']}/{r['noise_sigma']} | {r['total']}/{r['measurable']}/{r['failures']} | {fmt(r['mean'])} | {fmt(r['sd'])} | {fmt(r['p95'])}/{fmt(r['p99'])}/{fmt(r['max'])} | {fmt(r['formula_raw'])}/{fmt(r['rounded_candidate'])} | "+(f"{r['exceed_r']}/{r['exceed_ratio']:.3%}" if r['dataset']=='H' else 'H独立交叉检查见对应层')+" | 仅上述固定条件 |\n"
budget+='''
H公式列只用于透明对照，不反向确定r。既有2px真值验收限、4.131595px交点到连接弧预算和64px²语义预算均未自动当作r。

## 证据、hash与重放

复用已有实际raw测量，未重建海量网格；新分析逐图核对image_sha256与生成truth、物理映射及错误最大值，模型与manifest一致。C来自冻结v7正式stage诊断记录，H来自基线main整理后的stage记录。它们的历史producer源hash保留，不冒称是本轮新runner；Block4本轮H另复跑720次，并逐项对照检测/证据/方向/原因无差异。配置旧hash均为冻结v7，只新添temporal节点，geometry/corner/assignment生产值不变。

| 来源 | 路径 | SHA256 |
|---|---|---|
'''
for ds,v in l['provenance'].items():
 budget+=f"| {ds}历史原始记录 | `{v['record_path']}` | `{v['sha256']}` |\n| {ds}历史producer代码 | 定位/验证runner记录内 | `{', '.join(v['producer_code_sha256'])}` |\n| {ds}冻结配置 | frozen_detector_v7.yaml | `{', '.join(v['config_sha256'])}` |\n"
budget+='''
模型SHA256：`844853098082c0eae1972f744e084ca8b928233bf62b6c3933c6af4ada374109`。当前源码实际hash见grid每行及有效配置hash。`MARK_COMMIT_LABEL=08c5c44`仅基线标签；本轮未提交变化以源码hash标识。

原始每角误差：[C CSV](../../../build/block4-evidence/C-corner-errors.csv)、[H CSV](../../../build/block4-evidence/H-corner-errors.csv)；[分层统计JSON](../../../build/block4-evidence/localization-statistics.json)保留条件、样本量、来源hash。失败索引空（0例），不删分母。

网格扰动取C“w1440、圆角2px、σ1”实际已记录的四角有符号误差，按case_work_id字典序固定前32条；[C固定扰动CSV](../../../build/block4-evidence/C-fixed-noise.csv)列明每个ID及dx/dy，没有重新造高斯角点噪声、没有去均值。首帧也保留。独立[EXPERIMENTAL配置](../../../build/block4-evidence/EXPERIMENTAL-detector.yaml)含候选r，偏离档由工具固定网格逐段显式设置；不替换生产YAML。

```bash
python3 build/block4-evidence/measure_budget.py
build/block4/temporal_audit --experiment-grid --noise-csv build/block4-evidence/C-fixed-noise.csv --config build/block4-evidence/EXPERIMENTAL-detector.yaml --output build/block4-evidence/grid-final.jsonl
python3 build/block4-evidence/summarize_grid.py
```

已存在输出工具默认拒绝（退出1），重放应选新名或显式`--overwrite`指定本次产物。分析脚本在build证据目录，源码及命令均保留；删除build前须归档整个证据目录。所有新证据hash和大小见[manifest](../../../build/block4-evidence/artifact-manifest.json)。历史证据不回写。

## 平滑统计口径与完整对照

100px方框初始中心(720,540)，原图1440×1080；每段32次合法调用，时间7/14/28ms。x平移0/0.5/1/2/4/8px每次，旋转0/0.5/2/5°每次绕初始中心，平移和旋转分别跑；方向已知/未知，有/无C记录扰动；偏离0.5/1/2/4px均未批准，r实验候选2px。共480段、15,360次；对应错误、合法测量误删、unknown历史回填、raw改写各0；没有非法扰动输入。首次回退NO_HISTORY保留，未应用时alpha=null。T06/T07/I02另验empty无track。

RMSE为32×4个点对连续真值误差平方均值开根；抖动为静止段各物理角去32帧平均位置后的RMS。CSV另留31次相邻差分RMS（从第二帧开始），不是轨迹运动RMS。相对当前偏离为同一物理slot稳定/原始距离，统计128点均值/P95/P99/max；滞后是truth−stable沿x方向（平移）或当前角点正旋转切向（旋转）的投影，均值含首帧，正值表示落后，单位px，不称角度。静止段滞后仍可能含定位偏差，不推断运动。原始扰动非独立时间噪声，不从该比值推广视频收益。

完整CSV：[grid-summary.csv](../../../build/block4-evidence/grid-summary.csv)，原始逐调用：[grid-final.jsonl](../../../build/block4-evidence/grid-final.jsonl)。下表按段ID对应所有条件，K/U为方向已知/未知，C/0为记录扰动/无噪声；“回退”分项NO_HISTORY首帧、SMOOTHING_DEVIATION偏离回退。alpha及循环区间原始JSONL完整保留。

| 序列ID/条件（dt/运动/速率/方向/扰动） | r实验值 | 偏离实验档 | raw/stable真值RMSE | 静止抖动raw/stable/比值 | 当前偏离均值/P95/P99/max | 运动切向滞后均值px | 平滑 | 回退次数/原因 | 错对应/误删/unknown泄漏/raw改写 |
|---|---|---|---|---|---|---|---|---|---|
'''
for r in grid:
 cond=f"{r['segment']}: {r['dt_ms']}ms/{'x' if r['mode']=='translation' else 'rot'}/{r['rate']}{'px' if r['mode']=='translation' else '°'}/{'K' if r['known'] else 'U'}/{'C' if r['noisy'] else '0'}"
 budget+=f"| {cond} | {r['r_experimental_px']} 未批 | {r['deviation_experimental_px']} 未批 | {fmt(r['raw_truth_rmse'])}/{fmt(r['stable_truth_rmse'])} | {fmt(r['static_raw_jitter'])}/{fmt(r['static_stable_jitter'])}/{fmt(r['jitter_ratio'])} | "+'/'.join(fmt(r[k]) for k in ['deviation_mean','deviation_p95','deviation_p99','deviation_max'])+f" | {fmt(r['lag_mean_px']) if r['rate'] else 'N/A静止'} | {r['smoothing_calls']} | {r['fallback_calls']}: "+','.join(f'{k}={v}' for k,v in r['fallback_reasons'].items())+f" | {r['wrong_correspondence']}/{r['deleted_legal_measurements']}/{r['unknown_leaks']}/{r['raw_changed']} |\n"
budget+='''
## 支持范围、缺口与审批记录

定位C/H只覆盖固定16px笔画、完整结构、旋转和等比尺度，未覆盖更远距离、短边、任意仿射/透视、遮挡、非高斯强噪声或一般光照。快速未知旋转的四角对称性不保证持久物理身份；45°fixture明确歧义回退。14ms是系统目标而非本期性能验收；不新增计时/优化框架。没有V/Q标签，视频不用于预算统计或准确率结论。

审批状态：G-B两值**APPROVAL_PENDING**。建议范围仅上述合成条件及固定语义行为验证；未批准任何实拍定位概率/一般成像保证。用户尚未提供数值选择原文；数值栏留“未批准”。批准后在此追加原文/日期/单位/算法和数据版本/范围，保留本轮候选及未批历史。
'''
(docs/'block4_budget.md').write_text(budget)
plan=Path('src/tushenghao/docs/ref/Block4_输出稳定层_Codex完整实现方案_终稿.md').read_text()
# 原批准文本来源本方案（不是本轮额外授权）；置信度决策完整嵌入，不以链接代替四条/七步。
gc=plan[plan.index('**用户已确认：Block4不计算'):plan.index('将来评分只在')]
accept='''# Block4输出稳定层验收（WIP / G-B待批准）

2026-10-06。**实现及不依赖数值审批的验收已落实；Block4整体尚未完成。** 基线main与目标feat/temporal-stability均08c5c447348f46b9d033b90f109d48650745b4c6。G-B两个生产预算未批准，生产process仍NOT_READY；正式稳定视频回归BLOCKED。全视频raw非回归已通过，不能冒称完成批准配置稳定验收。

## 完成内容

保留core/config/preprocess/geometry/corners/pipeline六模块；新增稳定组件全在pipeline，共享FrameStamp在core，几何排序接口在corners，工具记录不进入算法库。冻结decode_stage无状态且未改；process/temporal_audit共享finalizeDecodedFrame，工具每帧只decode一次，生产文字源恒null。Detection/FrameResult直接字段和Detector四个调用签名、特殊成员签名未改，DisplayState仅按G-ABI追加value。

时间权重由14ms/.7构造tau，逐调用实际dt；double历史按当前原始slot保存，发布float再次检查再排序。选择参考是原始测量，平滑历史/显示历史独立。无/零/多关联均保留当前合法检测，最大四点面积/完全等面积输入顺序选单track。方向未知只用原始对原始循环误差区间，不回填方向；稳定输出复制当前载荷，仅替换点/bbox及当前方向排序映射。

empty立即无track，选择参考最多50ms且empty不续期；恢复首帧原始；零dt、歧义、非法几何/偏离回退原始并重建。非法图像/序列清选择、平滑、显示、输入元数据，下一合法低id首帧可重起；尺寸变化清历史。同尺寸换源/循环由调用方reset(InputChanged)。独立文字桥接按有效调用次数，第1～5缺失hold、第6清空，关闭透传不缓存。

D16：修复旧非法输入/序列return留下历史的问题，公共入口统一reset，新增I05及共享三状态恢复反例。NOT_READY清测量历史，同时保留合法输入帧戳作下一次序列检查，以保留旧重复帧负例，不缓存测量。D18：decode_audit仅改public_status=NOT_EVALUATED及说明，原始payload格式/链路保持；旧日志不回写。

CMake替换间接git rev-parse为MARK_COMMIT_LABEL，默认UNSPECIFIED；标签08c5c44仅表示基线，工作树真实源hash单独记录。新增3普通CTest，总19；Block5窗口/绘制/计时开关仍严格拒绝。G-B预算缺失可加载审计，开启内部稳定实例缺预算构造拒绝；生产关闭平滑仍不能绕过G-B。

## 决策记录

### G-ABI（已批准并落实）

批准依据为2026-10-06本方案§1.2/§4.3所载用户选A：“给DisplayState追加std::string value”；保持source_frame_id、age、is_held字段名称/类型/顺序，在后追加value。**这是ABI例外，FrameResult嵌套布局变化，不能混用旧ABI二进制。** 本轮mark_detector、app、旧16+新3测试、decode_audit/temporal_audit全部Release重新构建；独立H runner重新链接新库。类型见include/mark/detector_types.hpp，桥接见pipeline/display_history。没有重新请求既有批准。

### G-C（2026-10-06已确认，本期不实现）

'''+gc+'''
以上四条原因与七步为独立v0.2需求；本期没有新增评分函数/参数、confidence保持null，不以quality_flags当分数，不用历史软放行118个竞争拒绝帧，v0.1封存未改。

### G-B（数值待用户选择）

correspondence_uncertainty_px/max_smoothing_deviation_px均未批准，生产节点省略。实验统计已完成，r候选2.0px；偏离0.5/1/2/4档代价见[预算完整表](block4_budget.md)，推荐2px仅为提请选择，不视作批准。实验配置独立保存并标EXPERIMENTAL；等待只阻塞生产落地及正式稳定回归，不阻塞骨架、桥接、主动测试与文档。批准后追加原文/日期/值/单位/数据算法版本/范围，同步生产并重跑相关主动测试和一次正式全视频稳定验收。

## 验证方法与结果

Linux GNU13.3/CMake3.28.3/OpenCV4.6.0/OpenSSL3.0.13，C++17，无新依赖/测试框架。Release构建/CTest退出0，19/19通过；T01～T18（含T10b）19个命名用例，H01～H05五项，I01～I06六项，共30组主动用例均PASS。每组含多个断言，30是组数，不冒称断言数量。Debug对新三测试及配置负例/往返5/5通过（在最终新增旧排序等价/共享非法反例前已执行，最终这些新增反例由Release验证）。旧assert/空哨兵覆盖限制仍披露。

| 用例 | 实际输入/预期 | 执行结果与证据 |
|---|---|---|
'''
# 冻结测试表逐条搬入本轮验收，保留全部输入及预期，然后添加实际证据列。
a=plan.index('| T01 公式');b=plan.index('### 9.1',a)
for line in plan[a:b].splitlines():
 if line.startswith('|'):
  parts=line.split('|')[1:-1];name=parts[0].strip();expected=' / '.join(x.strip() for x in parts[1:])
  accept+=f"| {name} | {expected} | PASS；[Release最终日志](../../../build/block4-evidence/active-cases.log) |\n"
accept+='''
基本fixture方框Q(100,100)～(200,200)、原图1440×1080、14ms/.7/50ms、r2/偏离20（T14偏离2）只是机制值。旋转物理golden由固定矩阵独立计算；T09四点历史double保留，T10合法LT排序后按物理golden比较；T10b45°区间重叠不能选最小者。I01/I03/I04/I05使用真实渲染MARK→真实preprocess/geometry/decode→公共process；不向正式Detector传真值。I06及旧screen_order七个golden样例逐项查新接口等价，原算法能量/平局核心原样保留。

配置负例逐字段检查NaN/Inf/0/负值，alpha1、面积比错误、负hold、两预算缺失构造；YAML错误类型/空字符串/重复/未知、浮点hold与溢出；旧schema1冻结起点省略兼容、旧三字段缺失拒绝；往返使用非默认值覆盖全部新增字段。CLI缺参数、已有输出拒绝、视频打不开三项均退出1，见[audit负例](../../../build/block4-evidence/audit-negative-tests.json)。

```bash
cmake -S src/tushenghao -B build/block4 -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=08c5c44
cmake --build build/block4 -j 4
ctest --test-dir build/block4 --output-on-failure
cmake -S src/tushenghao/tools/block3_fixture -B build/block4-fixture -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=08c5c44
cmake --build build/block4-fixture --target block3_verify -j 4
build/block4-fixture/block3_verify build/block3-H-v1 src/tushenghao/docs/evidence/block3/frozen_detector_v7.yaml build/block4-evidence/H-regression.jsonl
build/block4/temporal_audit --video data/raw/marker_video.avi --config src/tushenghao/config/detector.yaml --output build/block4-evidence/temporal-production-pending.jsonl
python3 build/block4-evidence/compare_regression.py
```

H重新执行720/720正确、isolated720/720、wrong_valid0、退出0；与main基线整理记录逐case_work_id比对status/detections/measurements/diagnostics/truncated及correct标志无差异，见[H对照](../../../build/block4-evidence/H-comparison.json)。未重建任何图片/海量网格。原720通过ID全部保留。

全视频0～1675、SHA256 aa1219a7a7b702ea1265be8752853a267c982a651afa0f7846f516f9517c9ac7；fps产生时间，1676/1676记录、raw864/empty812，无raw状态/角点/bbox/方向/证据/原因/截断差异，confidence/marker_code空。生产G-B缺失，最终1676帧全NOT_READY、detections/tracks空、display=null；不是批准配置稳定视频回归。见[video对照](../../../build/block4-evidence/video-comparison.json)。该视频二进制源hash为记录中的实际hash，完成后又增补公共尺寸原因及测试等价检查；几何/decode生产源未改，不将基线标签当最终源码提交。排除标签/源与配置hash/耗时元数据，不排除任何raw几何或原因字段。

480段/15,360调用固定语义实验：合法测量误删0、错对应0、unknown回填0、raw改写0；每档偏离/滞后/回退分项完整见预算表及CSV/JSONL。不把运动算抖动，不以视频检出数作准确率。生产批准后正式视频还须逐track几何/偏离/恢复首帧验收，当前**BLOCKED_G-B_PENDING**。

## 踩坑记录

首次构建失败：新screen_order.hpp缺OpenCV类型声明；补直接include，不借传递include隐藏依赖。首轮CTest17/19：T13中心等号失败因为hypot(float,float)返回float，改明确double运算，不加epsilon放宽门限；hold4294967296被OpenCV截断为0，节点读取无法恢复原值，在新增hold字段原始YAML十进制token补溢出检查，旧geometry/corner规则未改。最初平滑发布float同时覆盖内部double历史，复核后改为独立published副本；double原始slot缓存不受对外排序/float影响。旧失败日志保留，不删除换绿。

第0步读到禁Git约束前已执行只读git status/log（未修改Git），这是执行边界偏离；发现后立即停止Git，改读.git/HEAD与refs确认main/目标一致。后续没有执行Git、提交、推送或切分支，CMake移除间接Git。此偏离如实保留，不声称全程零Git。基线只读hash证据见[baseline.json](../../../build/block4-evidence/baseline.json)。

## 已知限制与未闭合问题

1. G-B数值选择、生产落地及批准配置全视频稳定验收尚未完成，整体WIP。正式视频track几何/最大偏离/恢复检查BLOCKED，不是测试框架缺项。
2. D23原三L投影/观测面积规则和D25两份模型角边硬绑定按本方案范围外保留；本期不改geometry/preprocess/其余corners，不新增第三份绑定。
3. D12既有证据检查不重新fitLine验证L2最优或复验图像来源；稳定不伪造证据，不宣称已完成全部防篡改性质。
4. 一般仿射、短边、任意远距离/透视/光照覆盖不足；r统计支持范围见预算，未知方向快速旋转不保证持久身份。V/Q及置信度独立需求不因本期通过而闭合。
5. 旧其他assert与config_contract空哨兵不计实质覆盖；Block3整体仍WIP，保留历史。性能14ms未验收，Block5计时/绘制未实施，不能由稳定层宣称改善约97.65ms旧阶段性能。
6. 参考和实验脚本/大数据位于build/block4-evidence，删除build前须归档；必需文档及所有新源码导航已登记，旧冻结文档/evidence/v0.1未回写。

## 实际耗时与剩余动作

本轮约09:27开始阅读（首个基线hash快照09:29:42），实际区间见下表；时间来自本机Asia/Shanghai，分钟按活动区间估计，含工具运行/分析，不伪称秒级CPU工时。没有人为等待审批计入主动耗时。冻结6h40～10h30为原估计，本轮没有因超时削减范围。

| 步骤 | 开始/结束（2026-10-06） | 主动耗时 | 验证命令/退出码 | 剩余项 | 阻塞/批准依据 |
|---|---|---|---|---|---|
| 0 阅读/基线 | 09:27～09:30 | 约3min | 直接读取HEAD/refs、保存baseline/0 | 无 | main/目标同08c5c44；唯一未跟踪方案 |
| 1～2 状态/配置 | 09:30～09:34 | 约4min | Release configure/0；首次build失败记录保留 | 无 | G-ABI直接落地；无生产G-B默认 |
| 3～4 选择/对应/滤波 | 09:34～09:39 | 约5min | 首轮CTest失败→修复→19/19、0 | 无 | 对应/偏离仅fixture |
| 5 实验 | 09:39～09:44 | 约5min | measure_budget、grid、summarize/0 | 用户选择两值 | §6.3审批关卡 |
| 6～7 桥接/公共装配 | 09:34～09:47（与前几步交错） | 约5min已包含在前述活动中 | H01～H05、I01～I06/0；Debug5/5、0 | 获批后生产正常模式 | 生产语义源恒空，G-B仍待批 |
| 8 回归/文档 | 09:40～本轮结束（与实验交错） | 余下活动时段 | H720/0、video1676/0、raw比对/0、最终CTest/0 | 批准配置正式稳定视频 | G-B未批，不写整块完成 |

修改文件及所有产物实际hash/大小/可重放命令与退出码见[交付manifest](../../../build/block4-evidence/artifact-manifest.json)、[命令记录](../../../build/block4-evidence/commands.json)。人工批准记录待追加；本轮不操作版本管理。
'''
(docs/'block4_acceptance.md').write_text(accept)
print('budget lines',len(budget.splitlines()),'acceptance lines',len(accept.splitlines()))
