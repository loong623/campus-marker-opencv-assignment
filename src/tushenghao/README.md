# MARK 检测实现（Block4已完成，限批准范围）

当前内部阶段的 C、H 均为720/720；冻结v7全视频1676帧中864帧产生Detection。Block4稳定与独立文字桥接已实现；G-B已批准r=2px、偏离=2px，生产已落地；公共 `Detector::process()` 按当前可信测量正常返回 `DETECTED/NOT_DETECTED`，缺预算仍 `NOT_READY`，关闭平滑也不能绕过。视频未标注，检出数量不代表正确率；V按用户决定暂时跳过。

从[文档索引](docs/INDEX.md)、[Block3当前状态](docs/block3.md)、[适用边界与已知局限](docs/evidence/block3/05-L-observation-v5-v7/适用边界与已知局限.md)开始阅读。历史试验及失败版本见[证据索引](docs/evidence/block3/INDEX.md)，工具用法见[工具索引](tools/INDEX.md)。

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
├── tests/                   # 19个测试程序及固定反例data/
├── tools/
│   ├── audit/               # 当前帧/全视频阶段审计
│   ├── common/              # 工具共用摘要头
│   ├── block3_fixture/      # 生成、校准、验证、诊断、报表；独立CMake入口
│   └── synth_ref/           # 原参考生成器及其历史产物，保持原位
├── config/                  # 运行YAML和已核对模型坐标
└── docs/                    # 当前记录、冻结参考、架构与分批证据
```

库使用者包含 `<mark/detector.hpp>`。只有include是主库PUBLIC头路径；lib内部头供工程内app、测试与审计显式使用。内部头与实现共置，仍组成一个mark_detector库。详见[依赖划分](docs/architecture/include-dependency-analysis.md)和[源码迁移记录](docs/architecture/layout-reorganization.md)。

## 构建与测试

从仓库根目录执行，需要C++17、CMake、OpenCV和OpenSSL；独立fixture工具另需nlohmann_json。主检测库不直接依赖JSON/OpenSSL，不使用GTest。

```bash
cmake -S src/tushenghao -B build/tushenghao -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=08c5c44
cmake --build build/tushenghao -j 4
ctest --test-dir build/tushenghao --output-on-failure
build/tushenghao/marker_app --check-config
```

配置check只验证加载/校验/构造，不表示检测或稳定层验收完成。注册19个CTest程序（旧16+新3）；新增Block3检查在Release也主动报告失败，既有部分assert测试在Release的覆盖限制保留。

独立工具的构建入口、目标名保持不变：

```bash
cmake -S src/tushenghao/tools/block3_fixture -B build/block3-fixture -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=08c5c44
cmake --build build/block3-fixture -j 4
```

## 阶段审计

[runDecodePipeline](lib/pipeline/decode_stage.hpp)复用正式阶段模块，不读取合成真值。公共process按真实组件及完整预算就绪门控；离线审计通过decode_audit取得阶段Detection。

```bash
build/tushenghao/decode_audit data/raw/marker_video.avi 1200
build/tushenghao/decode_audit data/raw/marker_video.avi all src/tushenghao/docs/evidence/block3/frozen_detector_v7.yaml > build/video-audit.jsonl
```

默认配置为[detector.yaml](config/detector.yaml)，移动审计源文件后由CMake提供绝对配置路径，不依赖工作目录。冻结v1–v7的YAML和[ref/](docs/ref/)保留原路径、原内容。

输出四角由当前原图真实连续支持弧普通L2拟合求交；模型投影仅用于搜索。物理P0/P1/P2/P3对应L0/M1/L2/L3指定外角，输出顺序为屏幕LT/RT/RB/LB，bbox由当前四点计算。方向不唯一可为unknown，搜索截断不宣称唯一；marker_code和confidence保持空。

输入为BGR8，公共入口要求frame_id严格增加、timestamp_us非负且不倒退；reset清除序列与选择/平滑/显示三历史。工作像素中心映射为 `(work+0.5)/scale-0.5`。

## 当前结果与限制

H无噪声144/144、噪声576/576，原689和前705逐ID无回退，最大真值误差1.3462912原图px。全视频864个检测帧，相对v4保留581、增加283，10个旧检测帧因新增竞争未排除而保守拒绝。未输出812帧的互斥归因及逐帧证据见[最终回归](docs/evidence/block3/05-L-observation-v5-v7/H-video-v7.md)。

白阈值严格gray>200；325暗帧未进入白色掩膜，未标注不判漏检。未验证视频召回/误检率、Q人工定位误差、一般透视与任意尺度/亮度覆盖，也未承诺硬实时。模型绑定配置化和完整§9.1仍未闭合，整体Block3保持WIP。预算依据见[预算表](docs/block3_budget.md)，系统保证与不保证的范围见[适用边界](docs/evidence/block3/05-L-observation-v5-v7/适用边界与已知局限.md)。

最新文件整理与构建检查见[整理验证](docs/architecture/docs-tools-organization.md)。提交由用户review后操作。

## Block4使用与排错

[验收记录](docs/block4_acceptance.md)记录实际通过项与剩余项，[预算选择页](docs/block4_budget.md)包含定位统计、480段平滑对照和批准原文。基线标签由调用方传入`MARK_COMMIT_LABEL`，缺省`UNSPECIFIED`；CMake不调用Git。标签不是当前工作树提交，审计另外记录真实源码摘要。

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
| 时间/选择/三状态 | [temporal_stabilizer](lib/pipeline/temporal_stabilizer.hpp)、[T01～T18](tests/temporal_stabilizer_test.cpp) |
| 对应/偏离/稳定bbox | [temporal_geometry](lib/pipeline/temporal_geometry.hpp)，不调用原图证据validator |
| 未知几何环排序 | [screen_order](lib/corners/screen_order.hpp)、[历史等价测试](tests/screen_order_test.cpp) |
| 独立文字桥接 | [display_history](lib/pipeline/display_history.hpp)、[H01～H05](tests/display_history_test.cpp) |
| 公共装配/非法清理 | [stabilize_stage](lib/pipeline/stabilize_stage.hpp)、[Detector](lib/pipeline/detector.cpp)、[I01～I06](tests/temporal_integration_test.cpp) |
| 实验与视频审计 | [temporal_audit](tools/audit/temporal_audit.cpp)、[工具记录](tools/common/temporal_record.hpp) |
| 时序配置 | [强类型](include/mark/detector_config.hpp)、[加载/校验/导出](lib/config/config.cpp)、[负例](tests/config_error_test.cpp)、[往返](tests/config_roundtrip_test.cpp) |

当前预算候选只支持固定1440×1080、16px原图笔画、三工作尺寸、24旋转、0/2px圆角、灰度σ=0/1的C/H条件；不承诺任意距离、一般仿射/透视或光照变化。未知方向快速旋转的几何对应不提供持久物理身份保证。性能目标14ms未验收；Block5绘制/计时开关仍拒绝。视频864只是raw回归计数，无V/Q标签不报准确率。

Block4最终验证：Release19/19、相关Debug5/5；H720/720且逐例无差异；批准配置视频1676帧raw无差异，864track/812empty，应用平滑182，最大当前偏离1.994915px≤2。详细回退分项/批准/证据见[验收记录](docs/block4_acceptance.md)。整体Block3既有WIP、V/Q及性能限制保持原记录。
