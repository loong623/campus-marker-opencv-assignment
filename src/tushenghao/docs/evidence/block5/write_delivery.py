"""从实际已完成报告生成交付表，不将计划/未运行检查填PASS。"""
import datetime
import json
from archive_checks import ROOT, EVIDENCE, report_yaml, digest

required = ['baseline-check-03', 'debug-check-03', 'patched-debug-03',
            'patched-decode-all', 'patched-temporal-all', 'geometry-all-01-check',
            'decode-all-01-check', 'temporal-all-01-check', 'grid-01-check', 'grid-comparison',
            'video-invariants', 'H-comparison', 'H-final-comparison', 'route-b-cpp-check',
            'negative-cases', 'zero-frame-check']
for name in required:
    assert json.loads((EVIDENCE / 'comparison' / (name+'.json')).read_text())['result'] == 'PASS', name
baseline = report_yaml(EVIDENCE / 'runs/app-baseline-03/summary.yaml')
debug = report_yaml(EVIDENCE / 'runs/app-debug-03/summary.yaml')
manifest = report_yaml(EVIDENCE / 'runs/app-baseline-03/manifest.yaml')
commands = json.loads((EVIDENCE / 'archive/commands.json').read_text())
now = datetime.datetime.now().astimezone()


def milliseconds(summary, stage, key):
    value = summary['stages.'+stage+'.'+key]
    return 'N/A' if value == 'null' else f'{float(value)/1000:.6f}'


table = '|阶段|N|mean ms|median ms|p95 ms|p99 ms|max ms|\n|---|---:|---:|---:|---:|---:|---:|\n'
for stage in ['capture', 'preprocess', 'detect', 'decode', 'stabilize', 'process_total',
              'visualize', 'wait', 'export']:
    table += '|'+stage+'|'+baseline['stages.'+stage+'.N']+'|'+ '|'.join(
        milliseconds(baseline, stage, key) for key in ['mean_us', 'median_us', 'p95_us', 'p99_us', 'max_us'])+'|\n'
steps = '|步骤|实际命令首尾（北京时间）|记录命令墙钟秒合计|状态/修复/产物|\n|---|---|---:|---|\n'
labels = {0: '原始基线/Route B；P01–P06通过；step0/', 1: '内部通道/重置；修正fixture原因判断；tests/',
          2: '阶段计时与配置装配；tests/', 3: '统计/统一记录；tests/',
          4: 'App与三audit/verify；补齐声明头；tests/', 5: '四个主动测试；补齐verify声明头；tests/',
          6: 'Release23/23、相关Debug；完整Debug既有失败保留；tests/',
          7: '全视频/三audit/网格/H/失败与补强；runs/ comparison/',
          8: '输入/来源/重放/链接与归档；archive/'}
for step in range(9):
    entries = [c for c in commands if '/step'+str(step)+'-' in c['log'] and c['started_at']]
    start = min((c['started_at'] for c in entries), default='未单独记录')
    end = max((c['ended_at'] for c in entries), default='未单独记录')
    steps += f'|{step}|{start} → {end}|{sum(c["wall_seconds"] for c in entries):.3f}|{labels[step]}|\n'

acceptance = f'''# Block5 可观测性实施与验收

## 完成内容

实现已落地；验收状态见下表。全量Debug保留一项原基线失败，不宣称全部检查全绿；最终review与Git由用户处理。
依据[批准终稿](ref/Block5_可观测性_Codex完整实现方案_终稿.md)，第0步核对分支`feat/observability`与起点`b9cccd4a86ac5959a8c67559cde1d399ba29e046`，源树无先行代码差异，5个旧CSV仅CRLF/LF解释；旧归档70项hash一致。

Route B从物理P0–P3直接转float再共享规范排序，检查表示合法性，一次提交全部Detection，失败保留原measurement并整帧拒绝。P01–P06通过，当前视频无实际置换。D02无分配reset快照保留三种原因/次数/最后原因，外部来源null，避免Detector/Temporal双计，非法稳定输入诊断在清历史前复制并完整序列化。

六个lib模块内增加实例FrameRecord通道、九阶段状态/计时、唯一serializer/recorder/report；三audit共享装配，App独立baseline/debug与严格配置全链。公共布局/四调用及vector<string> diagnostics不变，原模型/几何/预处理/取证/语义数值锁定；r=2px、偏离=2px不变。RAW/STABLE仅当前，empty无旧有效图形，unknown不回填，HISTORY只文字，renderer克隆源图。离线不伪造实时延迟，视频导出开启明确报NOT_IMPLEMENTED。

当前实际生产代码摘要`{manifest['code_sha256']}`，外部提交标签只表示起点；[最终源清单](evidence/block5/environment/final_source_manifest.json)与原始/补丁前置清单分别保留。输入SHA256为`aa1219a7a7b702ea1265be8752853a267c982a651afa0f7846f516f9517c9ac7`，1440×1080、1676帧、fps70.408336；实际尺寸逐帧核验，见[输入身份](evidence/block5/environment/video_identity.json)。

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

正式性能使用baseline公共调用边界，Release/GCC13.3/OpenCV4.6/WSL2/i9-14900HX、OpenCV实际32线程，未改线程参数。N1676、首帧process_total={milliseconds(baseline,'process_total','first_frame_us')}ms仍在全量，吞吐{1676/(float(baseline['wall_us'])/1e6):.6f}fps；14ms为用户系统目标，**未达到**，不据此优化算法。

{table}
baseline实际诊断构造区间均值{float(baseline['diagnostics_construct_intervals.mean_us']):.6f}us、框架差额均值{float(baseline['framework_overhead.mean_us']):.6f}us。构造区间仅begin/event/output，详情复制已计入真实阶段与total；不能称所有日志成本。debug公共调用均值{milliseconds(debug,'process_total','mean_us')}ms，visualize均值{milliseconds(debug,'visualize','mean_us')}ms，export均值{milliseconds(debug,'export','mean_us')}ms；wait未执行。baseline/debug差值不解释为诊断精确开销。summary结束写盘与侧表补写见各run的run_export.yaml；capture/GUI/编码/等待/写盘均在公共process之外。

持续执行起点记录为12:10:44（北京时间），本表生成于{now.isoformat()}；命令墙钟与主动编写未分开计量，不能补造纯编写秒数。下表给实际可追溯的步骤首尾和命令耗时合计（含并行与嵌套，不等于整段主动时长）；初始四条原始基线命令无独立时间，明确null。后续归档结束时点见最终收据。

{steps}

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
'''
(ROOT / 'src/tushenghao/docs/block5_acceptance.md').write_text(acceptance)
index = '''# Block5 实施与验收证据

基线分支`feat/observability`，起点`b9cccd4a86ac5959a8c67559cde1d399ba29e046`。完整Debug旧失败保留；不以当前Release通过冒称所有旧assert通过。
总体结论见[验收页](../../block5_acceptance.md)，字段语义见[schema](../../diagnostics_schema.md)，运行入口见[README](../../../README.md)。

|证据|入口|
|---|---|
|原始/补丁源与金样|[step0目录](step0/)、[Route B比较](step0/route_b_comparison.json)、[C++只读复核](comparison/route-b-cpp-check.json)|
|最终源/输入/环境|[源hash](environment/final_source_manifest.json)、[工具链](environment/toolchain.json)、[视频身份](environment/video_identity.json)、[最终HEAD直接读取](environment/final_branch_state.json)|
|测试及所有失败日志|[tests](tests/)、[命令与退出码](archive/commands.json)、[主动34组及旧30组](tests/step7-final-active-02.log)|
|正式baseline|[summary](runs/app-baseline-03/summary.yaml)、[manifest](runs/app-baseline-03/manifest.yaml)、[有效配置](runs/app-baseline-03/effective_config.yaml)、[schema核验](comparison/baseline-check-03.json)|
|公共debug验证|[完整run](runs/app-debug-03/)、[逐帧对照](comparison/patched-debug-03.json)、[输出约束](comparison/video-invariants.json)、[schema核验](comparison/debug-check-03.json)|
|geometry/decode/temporal all|[geometry](runs/geometry-all-01/)、[decode](runs/decode-all-01/)、[temporal](runs/temporal-all-01/)、[共同raw](comparison/patched-decode-all.json)、[共同stable](comparison/patched-temporal-all.json)|
|网格原实验|[15360条run](runs/grid-01/)、[逐字段对照](comparison/grid-comparison.json)|
|H720|[最终逐例记录](comparison/H-final-regression.jsonl)、[旧金样对照](comparison/H-final-comparison.json)、[无损H输入](archive/H-input-manifest.json)|
|既有Debug失败|[原基线复现](comparison/legacy_debug_failure.json)、[归档二进制实际复现](comparison/archived-legacy-replay.json)、[完整Debug日志](tests/step7-evidence-debug-full-03.log)|
|实际错误用例|[19类CLI](comparison/negative-cases.json)、[零帧EOF](comparison/zero-frame-check.json)|
|完整性与清理|[archive目录](archive/)：artifact-manifest.json、final-archive-check-before/after-cleanup.json、final-archive-check-cleanup.json；最终命令另在final-archive-check-commands.json避免自引用|

01/02 App运行、首次缺产物H、首次错误verify与编译失败均保留作过程证据；正式App以03为准。性能只用baseline公共调用，Step0旧共享装配与audit无public总耗时。所有JSONL保留原文，不以摘要替代逐帧。

重放（先按README重新构建，报告路径必须新建）：

```bash
build/block5/observability_verify --compare src/tushenghao/docs/evidence/block5/step0/patched-baseline.jsonl src/tushenghao/docs/evidence/block5/runs/app-debug-03 --report /tmp/block5-replay-new.json
build/block5/observability_verify --check-run src/tushenghao/docs/evidence/block5/runs/geometry-all-01 --report /tmp/block5-geometry-new.json
build/block5/observability_verify --check-archive src/tushenghao/docs/evidence/block5 --report /tmp/block5-archive-new.json
```

[只读归档核查脚本](archive_checks.py)验证链接、当前源码、实际采样及全帧覆盖；[网格比较](compare_grid.py)、[视频约束](video_invariants.py)、[CLI错误](negative_cases.py)均为工具侧核验，不进入生产算法。重跑脚本需改新报告/运行名称，不覆盖本证据。[命令记录器](run_command.py)、[阶段顺序执行](run_stage_audits.py)、[零帧fixture](make_io_fixture.py)、[已有输入归档](archive_inputs.py)、[交付页生成](write_delivery.py)、[最终归档/限定清理](finalize_archive.py)全部登记，构建目录中无唯一必要脚本。

旧基线两个Linux平台二进制是不可从修改后源码直接重建的来源例外；普通新二进制不归档。H原输入在archive/H-existing-input.tar.gz，无损逐项SHA校验，原用户H目录保留。最终核验实际解析所有JSONL并校验manifest路径、大小与SHA；仅四个任务标识匹配的Block5目录清理，旧build、视频、Block3/4证据不清理。完整Debug失败仍在归档中并可复现。
'''
(EVIDENCE / 'INDEX.md').write_text(index)
docs = ['src/tushenghao/README.md','src/tushenghao/docs/INDEX.md','src/tushenghao/tools/INDEX.md',
        'src/tushenghao/docs/evidence/INDEX.md','src/tushenghao/docs/diagnostics_schema.md',
        'src/tushenghao/docs/block5_acceptance.md']
with (EVIDENCE / 'environment/delivery_documents_manifest.json').open('x') as stream:
    json.dump({'documents_sha256': {p: digest(ROOT/p) for p in docs}}, stream, indent=2)
    stream.write('\n')
print('PASS generated delivery from actual reports; legacy Debug failure explicitly retained')
