# README 历史全文（后续小修之前）

历史记录，当前操作见[项目 README](../../README.md)。旧验收数字不代表本轮结果；原命令仍按仓库根目录理解，供历史对照，不建议新人直接执行。下方完整保留原正文，仅修正迁移后的相对文件链接。

---

# MARK 检测实现（Block3/4/5 与 Final-fixes）

当前内部阶段的 C、H 均为720/720；冻结v7全视频1676帧中864帧产生Detection。Block4稳定与独立文字桥接已实现；G-B已批准r=2px、偏离=2px，生产已落地；公共 `Detector::process()` 按当前可信测量正常返回 `DETECTED/NOT_DETECTED`，缺预算仍 `NOT_READY`，关闭平滑也不能绕过。视频未标注，检出数量不代表正确率；V按用户决定暂时跳过。

从[文档索引](../../docs/INDEX.md)、[Block3当前状态](../../docs/block3.md)、[适用边界与已知局限](../../docs/evidence/block3/05-L-observation-v5-v7/适用边界与已知局限.md)开始阅读。历史试验及失败版本见[证据索引](../../docs/evidence/block3/INDEX.md)，工具用法见[工具索引](../../tools/INDEX.md)。

## 目录结构

```text
src/tushenghao/
├── include/mark/            # Detector公开接口、输入输出类型和配置类型
├── lib/
│   ├── core/                # 共享类型、模型、基础度量及预算检查
│   ├── config/              # 配置加载、校验和导出
│   ├── preprocess/          # 工作图和原图上下文准备
│   ├── geometry/            # L观察、父生成/验证、M/S补全
│   ├── corners/             # 原图角点取证、排序、语义及输出校验
│   └── pipeline/            # decode、三状态稳定/桥接与公共装配
├── app/main.cpp             # 配置检查应用入口
├── test_support/            # Block3像素fixture与Block4语义序列fixture
├── tests/                   # 23个测试程序及固定反例data/
├── tools/
│   ├── audit/               # 当前帧/全视频阶段审计
│   ├── common/              # 工具共用摘要头
│   ├── block3_fixture/      # 生成、校准、验证、诊断、报表；独立CMake入口
│   └── synth_ref/           # 原参考生成器及其历史产物，保持原位
├── config/                  # 运行YAML和已核对模型坐标
└── docs/                    # 当前记录、冻结参考、架构与分批证据
```

库使用者包含 `<mark/detector.hpp>`。只有include是主库PUBLIC头路径；lib内部头供工程内app、测试与审计显式使用。内部头与实现共置，仍组成一个mark_detector库。详见[依赖划分](../../docs/architecture/include-dependency-analysis.md)和[源码迁移记录](../../docs/architecture/layout-reorganization.md)。

## 构建与测试

从仓库根目录执行，需要 C++17、CMake ≥3.16、OpenCV 和 OpenSSL。
当前环境为 Ubuntu 24.04、GCC 13.3、CMake 3.28.3、OpenCV 4.6、OpenSSL 3.0.13；
这些版本是本轮实测环境，不代表所有版本均通过。

```bash
sudo apt-get update
sudo apt-get install --no-install-recommends build-essential cmake libopencv-dev libssl-dev
# 仅需要 JSON 头文件的独立工具使用；当前 Detector 不必安装：
sudo apt-get install --no-install-recommends nlohmann-json3-dev
# 可选的视频编码检查工具 ffprobe，App 运行不依赖它：
sudo apt-get install --no-install-recommends ffmpeg
```

独立fixture工具另需nlohmann_json。主检测库不直接依赖JSON/OpenSSL，不使用GTest。

```bash
cmake -S src/tushenghao -B build/tushenghao -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=08c5c44
cmake --build build/tushenghao -j 4
ctest --test-dir build/tushenghao --output-on-failure
build/tushenghao/marker_app --check-config
```

配置check只验证加载/校验/构造，不表示检测或稳定层验收完成。当前注册23个CTest程序；新增Block3检查在Release也主动报告失败，既有部分assert测试在Release的覆盖限制保留。

独立工具的构建入口、目标名保持不变：

```bash
cmake -S src/tushenghao/tools/block3_fixture -B build/block3-fixture -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=08c5c44
cmake --build build/block3-fixture -j 4
```

## 阶段审计（Block3历史命令）

[runDecodePipeline](../../lib/pipeline/decode_stage.hpp)复用正式阶段模块，不读取合成真值。公共process按真实组件及完整预算就绪门控；离线审计通过decode_audit取得阶段Detection。

```bash
build/tushenghao/decode_audit data/raw/marker_video.avi 1200
build/tushenghao/decode_audit data/raw/marker_video.avi all src/tushenghao/docs/evidence/block3/frozen_detector_v7.yaml > build/video-audit.jsonl
```

默认配置为[detector.yaml](../../config/detector.yaml)，移动审计源文件后由CMake提供绝对配置路径，不依赖工作目录。冻结v1–v7的YAML和[ref/](../../docs/ref/)保留原路径、原内容。

输出四角由当前原图真实连续支持弧普通L2拟合求交；模型投影仅用于搜索。物理P0/P1/P2/P3对应L0/M1/L2/L3指定外角，输出顺序为屏幕LT/RT/RB/LB，bbox由当前四点计算。方向不唯一可为unknown，搜索截断不宣称唯一；marker_code和confidence保持空。

输入为BGR8，公共入口要求frame_id严格增加、timestamp_us非负且不倒退；reset清除序列与选择/平滑/显示三历史。工作像素中心映射为 `(work+0.5)/scale-0.5`。

## 当前结果与限制

H无噪声144/144、噪声576/576，原689和前705逐ID无回退，最大真值误差1.3462912原图px。全视频864个检测帧，相对v4保留581、增加283，10个旧检测帧因新增竞争未排除而保守拒绝。未输出812帧的互斥归因及逐帧证据见[最终回归](../../docs/evidence/block3/05-L-observation-v5-v7/H-video-v7.md)。

白阈值严格gray>200；325暗帧未进入白色掩膜，未标注不判漏检。未验证视频召回/误检率、Q人工定位误差、一般透视与任意尺度/亮度覆盖，也未承诺硬实时。模型绑定配置化和完整§9.1仍未闭合，整体Block3保持WIP。预算依据见[预算表](../../docs/block3_budget.md)，系统保证与不保证的范围见[适用边界](../../docs/evidence/block3/05-L-observation-v5-v7/适用边界与已知局限.md)。

最新文件整理与构建检查见[整理验证](../../docs/architecture/docs-tools-organization.md)。提交由用户review后操作。

## Block4使用与排错（历史接口说明，当前命令见Block5）

[验收记录](../../docs/block4_acceptance.md)记录实际通过项与剩余项，[预算选择页](../../docs/block4_budget.md)包含定位统计、480段平滑对照和批准原文。基线标签由调用方传入`MARK_COMMIT_LABEL`，缺省`UNSPECIFIED`；CMake不调用Git。标签不是当前工作树提交，审计另外记录真实源码摘要。

```bash
cmake -S src/tushenghao -B build/block4 -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=08c5c44
cmake --build build/block4 -j 4
ctest --test-dir build/block4 --output-on-failure
build/block4/temporal_audit --video data/raw/marker_video.avi --config src/tushenghao/config/detector.yaml --output build/block4-evidence/new-temporal.jsonl
```

`--video`、`--config`、`--output`都必填，路径按调用目录解析；工具可创建输出父目录，已有输出或配套有效配置默认拒绝，只有显式`--overwrite`覆盖本次指定产物。有效配置另写`<output>.effective.yaml`。输入、配置、视频打开或写记录失败退出1；退出0只说明行为审计完成，批准状态及检测结果见记录，不能代替正式验收。工具逐帧只调用一次decode，记录其raw集合/完整证据，再经共享finalize装配；`integration=SHARED_STAGE_ASSEMBLY`，公共process等价性由集成测试验证。decode_audit仅评阶段，`public_status=NOT_EVALUATED`。

时间来自视频fps生成的非负微秒，平滑使用实际时间差；`reference_dt_ms=14`、`reference_alpha=.7`构造tau，`history_max_gap_ms=50`，中心门限为原始bbox对角线`.5`，四点面积比范围`.5～2`。预算`correspondence_uncertainty_px`与`max_smoothing_deviation_px`单位均为原图px，当前用户已批准两者均为2.0px；缺失仍未就绪，不补经验值。旧schema=1可省略六个新冻结起点，有效导出写全；旧开关/hold三个字段仍必填。`max_hold_frames`为非负十进制整数，类型与溢出严格检查。

Detection保留当前原始测量；最多一个TrackResult引用其中合法索引，只更新稳定点、由点重算的bbox及当前可信方向在重新排序后的映射；confidence/marker_code生产恒空。当前empty立即无track，恢复首帧原始，下一连续有效帧才平滑。选择参考按上次原始有效时间最多保留50ms，empty不能续期。unknown始终unknown。DisplayState只含独立文字及来源/年龄，生产没有语义源，恒空；显示桥接默认关闭，A/B仅在合成测试验收5/6边界。

同尺寸换视频、循环或更换相机时必须由调用方`detector.reset(mark::ResetReason::InputChanged)`；公共接口没有source_id，不能自动辨别。尺寸改变自动清历史；非法图像/帧号/时间清全部历史，下一合法帧可以重新开始。DisplayState追加value是用户已批准ABI例外，FrameResult嵌套布局随之变化，库、app、测试、工具须全部重建。

| 原因 | 含义与处理位置 |
|---|---|
| PIPELINE_BUDGET_MISSING | 公共配置缺assignment、角预算或G-B，NOT_READY；看预算记录，不靠关闭平滑放行 |
| INPUT_FORMAT / INVALID_STAMP / INVALID_SEQUENCE | 图像格式或元数据非法，INVALID_INPUT并全清；新段调用reset |
| INPUT_CHANGED / HISTORY_EXPIRED | 尺寸变更或距上次原始有效帧超过50ms，从当前原始点重建 |
| EMPTY_CURRENT / SMOOTHING_HISTORY_EMPTY | 当前空无track；失检恢复不跨段滤点 |
| ASSOCIATION_FAILED / ASSOCIATION_AMBIGUOUS | 零/多可能关联，重选最大四点面积并原始透传，保留raw全体 |
| ZERO_DT / CORRESPONDENCE_AMBIGUOUS | 同时间或对应区间重叠，回退当前原始并重建 |
| SMOOTHING_DEVIATION / NON_CONVEX_OR_DEGENERATE / OUT_OF_BOUNDS | 平滑检查失败，回退当前原始，禁止裁点 |
| INVALID_SCREEN_ORDER / INVALID_ORIENTATION / INVALID_RAW_BBOX | 内部候选防御失败，INVALID_INPUT；排查上游编排，不能静默删候选 |

| 修改目标 | 入口与测试 |
|---|---|
| 时间/选择/三状态 | [temporal_stabilizer](../../lib/pipeline/temporal_stabilizer.hpp)、[T01～T18](../../tests/temporal_stabilizer_test.cpp) |
| 对应/偏离/稳定bbox | [temporal_geometry](../../lib/pipeline/temporal_geometry.hpp)，不调用原图证据validator |
| 未知几何环排序 | [screen_order](../../lib/corners/screen_order.hpp)、[历史等价测试](../../tests/screen_order_test.cpp) |
| 独立文字桥接 | [display_history](../../lib/pipeline/display_history.hpp)、[H01～H05](../../tests/display_history_test.cpp) |
| 公共装配/非法清理 | [stabilize_stage](../../lib/pipeline/stabilize_stage.hpp)、[Detector](../../lib/pipeline/detector.cpp)、[I01～I06](../../tests/temporal_integration_test.cpp) |
| 实验与视频审计 | [temporal_audit](../../tools/audit/temporal_audit.cpp)、[工具记录](../../tools/common/temporal_record.hpp) |
| 时序配置 | [强类型](../../include/mark/detector_config.hpp)、[加载/校验/导出](../../lib/config/config.cpp)、[负例](../../tests/config_error_test.cpp)、[往返](../../tests/config_roundtrip_test.cpp) |

当前预算候选只支持固定1440×1080、16px原图笔画、三工作尺寸、24旋转、0/2px圆角、灰度σ=0/1的C/H条件；不承诺任意距离、一般仿射/透视或光照变化。未知方向快速旋转的几何对应不提供持久物理身份保证。Block4阶段性能目标14ms未验收；本轮开关已实现，当前结果见Block5验收。视频864只是raw回归计数，无V/Q标签不报准确率。

Block4最终验证：Release19/19、相关Debug5/5；H720/720且逐例无差异；批准配置视频1676帧raw无差异，864track/812empty，应用平滑182，最大当前偏离1.994915px≤2。详细回退分项/批准/证据见[验收记录](../../docs/block4_acceptance.md)。整体Block3既有WIP、V/Q及性能限制保持原记录。

## Block5 离线可观测性

依赖沿用C++17、CMake、OpenCV、工具OpenSSL；无新增库。工作目录是仓库根。输入可放任意位置，通过`--video`指定，不依赖个人目录。

```bash
cmake -S src/tushenghao -B build/block5 -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=b9cccd4
cmake --build build/block5 -j 4
ctest --test-dir build/block5 --output-on-failure
build/block5/marker_app --check-config --config src/tushenghao/config/detector.yaml
build/block5/marker_app --video data/raw/marker_video.avi --config src/tushenghao/config/detector.yaml --run-dir new-runs/baseline --mode baseline
build/block5/marker_app --video data/raw/marker_video.avi --config src/tushenghao/config/detector_debug.yaml --run-dir new-runs/debug --mode debug
build/block5/marker_app --video data/raw/marker_video.avi --config src/tushenghao/config/detector_verification.yaml --run-dir new-runs/verification --mode debug --run-purpose verification
build/block5/geometry_audit --video data/raw/marker_video.avi --config src/tushenghao/config/detector.yaml --run-dir new-runs/geometry
build/block5/decode_audit data/raw/marker_video.avi all src/tushenghao/config/detector.yaml --run-dir new-runs/decode
build/block5/temporal_audit --video data/raw/marker_video.avi --config src/tushenghao/config/detector.yaml --run-dir new-runs/temporal
build/block5/observability_verify --check-run new-runs/debug --report new-runs/debug-check.json
build/block5/observability_verify --compare src/tushenghao/docs/evidence/block5/step0/patched-baseline.jsonl new-runs/debug --report new-runs/comparison.json
build/block5/observability_verify --check-archive src/tushenghao/docs/evidence/block5 --report /tmp/block5-archive-check.json
```

从任意CWD执行时，将程序、video、config、run-dir参数替换为绝对路径；模型相对路径继续相对配置目录解析。上述所有输出目录/报告必须是新路径，不能重复覆盖证据。生产无参打印帮助并非零退出。基线完整处理每帧并开启稳定，关闭GUI/等待/详情；debug采样只改变记录和证据，不能跳算法帧。配置仍为root/input/preprocess/detector/geometry/temporal/output/debug；CLI覆盖路径、唯一mode、报告用途run-purpose及下文新参数，最终effective_config可重载。

三audit使用同一schema/统计，分别运行geometry、decode、temporal实际范围，process_total保持NOT_EXECUTED。decode旧帧列表仍表示执行子集，all才全帧；temporal的`--output PATH`兼容到新`PATH.run/frames.jsonl`，原PATH只写说明，`--overwrite`也不覆盖旧归档。实验网格保留原480段×32，使用`--experiment-grid --noise-csv ... --config ... --run-dir ...`，不更改生产预算。

错误先看stderr和run内FAILED.json、命令退出码；summary的incomplete/failed不能忽略。源时间戳用于算法，steady_clock用于成本；离线实时元数据null。14ms来源是用户系统目标，正式比较只用public_call的process_total，读图/绘制/等待/写盘另报。RAW/STABLE为当前图层；empty不绘旧框，unknown不回填，held仅HISTORY文字。证据详情复用原始取证，不从稳定点伪造交点。

Block5 历史版本未实现视频导出；当前 Final-fixes 已实现，使用方法见下文。D23原三L面积规则、D25角边绑定、一般透视/远距离/置信度仍是原范围限制。旧geometry_matcher_test的Debug assert在原基线也失败，详见[Block5验收](../../docs/block5_acceptance.md)，不能把Release23/23解释成该旧assert已验证。

本轮证据见[导航](../../docs/evidence/block5/INDEX.md)、[schema](../../docs/diagnostics_schema.md)。固定build仅四目录，任务标识匹配、必要产物归档、JSONL/hash/链接与重放核查通过后才清本任务build；旧build、视频、Block3/4归档不清理。后续重放可重新构建同路径，不能依赖被清目录中的唯一证据副本。

## Final-fixes 使用与验收

本轮按 A→B→C→D 完成代码及自动验证。Release 与 Debug 均为原有 23/23 个 CTest；
历史 c7fffe4 的 Debug 22/23 是旧 matcher fixture 问题，当前正例通过真实模型与观测层构造。
详细结果见[本轮验收](../../docs/final-fixes_acceptance.md)及[永久证据导航](../../docs/evidence/final-fixes/INDEX.md)。
有屏显示、人工播放器确认仍待用户执行；干净 Linux 因无环境入口按用户决定记为未验证。

从仓库根目录执行。输入路径由调用者提供，配置模型路径相对 YAML 所在目录解析。
输出 run-dir 和验证 report 均须为新路径，不覆盖旧证据。

```bash
cmake -S src/tushenghao -B build/new-user -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=c7fffe4-final-fixes-working-tree
cmake --build build/new-user -j4
ctest --test-dir build/new-user --output-on-failure
VIDEO=/absolute/path/to/marker_video.avi
build/new-user/marker_app --check-config --config src/tushenghao/config/detector.yaml
build/new-user/marker_app --check-config --config src/tushenghao/config/detector_debug.yaml
build/new-user/marker_app --check-config --config src/tushenghao/config/detector_verification.yaml
build/new-user/marker_app --video "$VIDEO" --config src/tushenghao/config/detector.yaml --mode baseline --run-dir new-runs/baseline
build/new-user/marker_app --video "$VIDEO" --config src/tushenghao/config/detector_debug.yaml --mode debug --display --run-dir new-runs/debug
build/new-user/marker_app --video "$VIDEO" --config src/tushenghao/config/detector_verification.yaml --mode debug --run-purpose verification --export-video --expected-frames 1676 --run-dir new-runs/verification
build/new-user/observability_verify --check-run new-runs/verification --expected-frames 1676 --report new-runs/check-full.json
build/new-user/observability_verify --compare src/tushenghao/docs/evidence/block5/step0/patched-baseline.jsonl new-runs/verification --expected-frames 1676 --report new-runs/regression-full.json
build/new-user/final_fixes_verify video --input new-runs/verification/overlay.mp4 --expected-frames 1676 --width 1440 --height 1080 --report new-runs/video-readback.json
ffprobe -v error -select_streams v:0 -show_entries stream=codec_name,codec_tag_string,width,height,r_frame_rate,nb_frames -of json new-runs/verification/overlay.mp4
```

`--display`、`--export-video` 是 debug 模式的无值开关，baseline 冲突、重复及未知参数均拒绝。
无 DISPLAY/WAYLAND_DISPLAY 或独立 GUI 探测进程失败时显示降级，继续离屏处理；
有屏时 q/ESC 提前停止，保留记录及 INCOMPLETE.json，非零退出，不发布完整视频。
探测进程与主进程隔离，Qt 初始化 abort 不会直接杀死主进程。

视频固定请求 MJPG、FFmpeg 后端及 `overlay.mp4`，不静默切换编码。
逐帧写入当前 overlay，不受详情采样间隔影响，源尺寸和源 FPS 保持一致；
关闭 writer 并完整读回验证后才把 `.partial.mp4` 改名为 `overlay.mp4`。
损坏输入或提前停止保留 partial。FFmpeg 在 MP4 中可把 tag 改成 mp4v；
实际编码检查看 `codec_name=mjpeg`，不能只看 tag 或扩展名。
源 FPS 用于媒体时间轴，不表示检测处理速度。

baseline 输出 manifest/effective config/summary/run_export，不输出帧详情、窗口或视频；
debug 采样记录，verification 全帧记录。图片仅在 export_evidence 开启时写入。
`--expected-frames N` 必须为正整数；不指定时使用可靠的正整数容器帧数，
无可靠元数据则完整性未验证并非零退出，提示提供期待数。
提前 EOF、子集缺失、人工停止保留已处理帧和 INCOMPLETE.json；
零帧和打开失败写 FAILED.json，不制造有效成功 summary。
合法子集只声明 explicit_subset，不宣称全视频回归。

终端保留原英文技术行，例如 `run=baseline-final frames=1676 fingerprint=c1785fb03a54ed25 incomplete=0`，
随后输出中文状态、读取/处理/检测数量、范围/覆盖、完整性原因、处理平均/p95、运行总耗时及测得瓶颈。
瓶颈仅比较四个算法阶段的已测样本均值；关闭计时或无有效样本时明确不提供该统计。
错误定位先看退出码和 stderr，再看 FAILED.json/INCOMPLETE.json、manifest.environment 与 summary。
帧内 Export 槽在完成补记前为 NOT_EXECUTED，实际结果以 export_timings.jsonl 关联补记；
计时关闭为 DISABLED/null，侧表及收尾成本归 run_export。

本轮实测环境与必需安装命令见上文。FFmpeg/GUI 的 OpenCV 后端属于环境条件，
ffprobe 是可选核验工具；没有显示服务也可运行三模式。新路径复制构建使用本机依赖，
不能据此声称已在干净 Ubuntu 验证。历史 Block3/4/5 证据与冻结参考保持原内容。

输入路径不存在或 CLI/配置在创建 run 前被拒绝时，只保留退出码与 stderr，不保证有 run 目录；
文件存在但解码器无法打开、以及已打开却零帧时，FAILED 生命周期已实跑验证，不生成成功 summary。

## Path A 锚点竞争修复（2026-10-06）

三L只用独立合法拓扑的实测凹角，完整保留合法多锚点及来源，合法失败竞争仍empty。原23项加2项，Release/Debug均25/25；manual_validation_check经用户批准修正fixture并保留目的。
Release全1676帧及专项核查通过：864→976成功，118旧PathA恢复112，零旧成功退化、零A外状态变化；20旧成功raw四角改变需review，方向零变化。Debug完整1676及逐帧公共比较均PASS，同新指纹0d2d2e63aef753bc，最终状态见[专项验收](../../docs/path_a_acceptance.md)和[施工日志](../../docs/path_a_fix_log.md)。PathB／564无假设延期fix2，不声称P0整体解决。

```bash
cmake -S src/tushenghao -B build/final-fixes-path-a-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/final-fixes-path-a-release -j4
ctest --test-dir build/final-fixes-path-a-release --output-on-failure
build/final-fixes-path-a-release/path_a_verify --baseline-run src/tushenghao/docs/evidence/final-fixes/path-a/baseline --candidate-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-release --config src/tushenghao/config/detector_verification.yaml --expected-frames 1676 --report-dir new-runs/path-a-review
build/final-fixes-path-a-release/path_a_verify render --candidate-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-release --video data/raw/marker_video.avi --review-list src/tushenghao/docs/evidence/final-fixes/path-a/report-release/review_required.csv --output-dir new-runs/path-a-images
```

报告/证据图目录必须是新路径。Debug用对应debug构建目录与`CMAKE_BUILD_TYPE=Debug`；逐字段raw差异、tracks/display历史差异分开报告，投影残差不是真值误差。132帧证据[review索引](../../docs/evidence/final-fixes/path-a/step6/review-approved.csv)已获用户全部通过确认，旧运行金样未覆盖。
