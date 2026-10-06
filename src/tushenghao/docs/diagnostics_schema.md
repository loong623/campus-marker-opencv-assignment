# Block5 诊断 schema 1

记录版本 `record_schema_version=1` 与配置 `schema_version=1` 独立。旧 Block4 JSONL 只读适配，不回写或补造计时。

## 帧、范围与当前结果

`FrameRecord` 是内部实例通道，公共 `FrameResult.diagnostics` 仍为 `vector<string>`；Detector只增加私有friend，公共四调用及Detection/FrameResult/DisplayState布局不变。内部消费顺序为prepare→process→take；重复prepare/take、未取记录就继续process主动失败。未使用内部访问器的普通公共消费继续工作。

身份含run_id、十进制字符串frame_id/source_timestamp_us、timestamp_source及timestamp_recipe；原图/工作图尺寸为整数数组。`execution_scope` 为full/geometry/decode/temporal。geometry的result_status为null，geometry_scope_result为READY/NOT_READY/INVALID_INPUT，不把几何假设称DETECTED。audit的public_status为NOT_EVALUATED。

可选当前result含原始detections、稳定tracks和独立display；详情复用PreparedFrame、WhiteComponent、ShapeObservation、GeometryBatch、CornerMeasurement和TemporalDiagnostics。只在选中帧分配/复制，不重拟合或改写原始证据。实验网格详情的truth仅工具填写，Detector不消费。

## 计时

九个固定顺序槽：capture、preprocess、detect、decode、stabilize、process_total、visualize、wait、export。MEASURED有有限非负elapsed_us（保留小数），DISABLED/SKIPPED/NOT_IMPLEMENTED/NOT_EXECUTED值为null。未进入stage不填0。计时用steady_clock，算法timestamp由输入视频fps生成，二者不混用。

App包围一次真正公共调用，替换内部近似total，`process_total_boundary=public_call`；没有两项相加。process包含输入/预算检查、阶段、结果及内部诊断装配；不包括prepare/take、指纹、渲染、等待和文件写。audit的process_total为NOT_EXECUTED。

`diagnostics_construct_intervals_us`只计实际包围的context.begin、事件push和context.output区间，**不是全部诊断开销**。细节复制在对应stage和process内；未单独隔离的复制成本不猜算，不把debug−baseline称精确诊断成本。框架差额只在四算法stage与total均MEASURED时计算。runner的bookkeeping计prepare/take/指纹及关联装配实测区间。

每帧导出先写frames.jsonl，Export槽仍为NOT_EXECUTED，再将实际写入耗时写到`export_timings.jsonl`，以run_id/frame_id唯一关联，汇总替换一个状态/样本。侧表自身不递归计时，其写入和finishRun摘要写入归run_export；`run_export.yaml`补全最终实测值，该补充报告自身写入不再递归计时。EOF失败read单列manifest，不算算法帧。

统计N、mean、median（偶数两项平均）、nearest-rank p95/p99、max；空统计null，首帧单列仍在全量，慢帧不删除。各stage分位不相加。14ms@1440×1080是用户系统目标，只对照公共process_total，不是作业下限或保证。

## 原因、整数与JSON

事件含stage、ReasonCode、detail原文、可选source_frame_id/hypothesis_id/component_id、occurrences。旧不可细分文本归OtherRecordedReason，不由字符串猜遮挡等物理原因。reset使用无分配饱和计数；外部来源null，同一语义调用不在Detector和Temporal双计。非法finalize先保留本次TemporalDiagnostics，后清三历史。

所有uint64/int64写JSON十进制字符串，small enum/尺寸为数字；float/double有限数用classic locale和17位精度。非有限写null并追加NonFiniteField事件；合法0不丢。UTF-8保留，控制字符/引号/反斜线转义。reader只供测试/验证，拒重复键、尾随内容、截断、非法转义、非有限number，支持Unicode代理对；不进入Detector。

summary不建全视频几何副本或frames文件。frame/evidence仅选中输入写详情，但所有实际算法帧都计统计。summary计数不被detail采样稀释。旧逐父affine/assignments长trace仅详细选中帧构造；准入/回退原因不删除。

offline实时字段全部null，realtime.applicability=not_applicable；不制造帧槽覆盖、曝光延迟或enqueue时钟差。纯渲染clone原图，empty/INVALID_INPUT/NOT_READY无有效图形；unknown无P标签，历史仅HISTORY/source/age文字。错帧证据先验证，由runner附事件，renderer不改const记录。

## 配置与指纹

旧debug.timing_enabled/draw_candidates和output.show_window/show_held_state是唯一生效旧开关；新App配置存level、范围、图层、模式、输出目录、导出和回放速率。boolean只接受0/1；范围配置采用非负32位整数并在原始token上检查溢出（记录frame_id仍支持uint64）。baseline拒详细/GUI/等待/证据冲突，关闭timing也拒绝。CLI仅覆盖路径和模式，effective_config导出最终值。export_video=1明确NOT_IMPLEMENTED。

结果指纹FNV1a64版本1：初值14695981039346656037、乘1099511628211、uint64溢出。id(uint64)、timestamp(int64位模式)、status(uint32)，raw vector长度(uint64)及每项category/quality(uint32)、corners/bbox(IEEE float32)、orientation/marker_code/confidence，track vector长度/索引(uint64)/同Detection，display可选。optional先0/1；string长度uint64后UTF8；orientation四int32；marker_code为scheme/value(int32)；display为source/age(uint64)、held(1byte)、value(string)。所有整数little-endian。排除diagnostics/timing/runmeta。App文件指纹为FNV1a64+size，明确非密码学摘要，归档工具补SHA256。结果指纹仅连接summary基线，不能代替逐帧比较。

## 报告与失败

新run目录必须不存在；固定manifest.yaml/effective_config.yaml/summary.yaml，按需frames.jsonl/export_timings.jsonl/evidence。生命周期非法、文件打开/写入、零帧或提前退出非零；失败标记FAILED.json，不能把不完整run当全视频PASS。CPU/系统/编译器/OpenCV/build type/线程/源代码/输入/配置/模型与时戳公式入manifest；不可获取项明确UNKNOWN。

[验收记录](block5_acceptance.md)与[证据导航](evidence/block5/INDEX.md)记录实际运行及限制。

组件详情保留工作图bbox和完整原轮廓；观测保留turns、anchor与实际L拓扑候选，hypothesis assignments始终引用本帧组件。last_reset_reason及last_reset_source_frame_id保留最后语义调用，外部来源为null。验收验证工具检查来源引用，不重新拟合。

`--run-purpose verification`只标明debug报告用途，不增加生产模式；正式baseline仍summary，验证用途debug独立使用公共process产生全帧记录，不能冒称baseline开销。

原角点详情的`physical`来自CornerEvidence自身枚举，`measurement_error_px`直接保存原error_（两线平均残差之和），不从稳定点计算。旧金样未包含error_，verify对共同证据字段exact比较，另核新增error_与原两线均值之和相等。直接Temporal的非法update也保留其详情，不以是否执行geometry决定丢弃已取得的诊断。
