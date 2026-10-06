# Block3 fixture复用检查（2026-10-05）

状态：已有核心构建/测试、小样本与C隔离实验已运行；H和正式阶段验收未运行。
不改 `tools/synth_ref/` 参考源码/旧CLI/verify。独立adapter在 `tools/block3_fixture/`，通过CMake显式链接 `synthetic_gen_core`；可用 `-DSYNTH_REF_SOURCE=/实际路径` 指定原库checkout。

用户已确认 `config/marker_geometry.yaml` 坐标和M160/L416/S64。adapter直接读该YAML，每个多边形面积与坐标计算核对；不修改旧sources.json的待审历史状态，也不伪造旧工具approval.json。

| 能力 | 实際复用/实现 | 实测 |
|---|---|---|
| 连续模型/变换 | `sg::Piece`、`transform_polygon`；显式中心/2px-u矩阵 | C 240张1440×1080原图；H plan 240条 |
| 原图渲染 | `sg::rasterize(scene,1440,1080)`；旧8×8覆盖规则 | PNG读回BGR8尺寸/像素一致，hash记录 |
| 圆角 | 最小adapter相切圆弧，旋转前原图半径2px，每90°16段；凹凸用相同相切规则 | 15°小样本16/32段比较，最大灰度差4（一个子采样覆盖量级）；几何弓高上界0.0024090876px。只验证该小样本，未声称全网格渲染收敛 |
| 噪声 | 固定mt19937/Box-Muller-v1，零均值σ=1灰度；lround后截断再复制BGR | 小样本seed11，C seed11/22/33/44；H seeds55/66/77/88只计划 |
| 真值/记录 | 名义未圆角结构交点、输入/实例mask SHA-256、条件manifest；核心hash/environment复用 | 隔离runner仅读工具实例mask提供assignment，正式Detector没有真值入口 |
| 验证边界 | 独立PNG读回、模型面积、相切轮廓合法性、支持边身份核查 | 旧verify未替1440/噪声/圆角条件背书 |

## 命令和退出码

| 命令 | 退出码/结果 |
|---|---|
| `cmake -S src/tushenghao/tools/synth_ref -B build/block3-synth-ref -DCMAKE_BUILD_TYPE=Release`；构建 | 0 |
| `ctest --test-dir build/block3-synth-ref --output-on-failure` | 0；7/7（旧库有限测试，不是重跑38,784项旧网格） |
| `cmake -S src/tushenghao/tools/block3_fixture -B build/block3-fixture -DCMAKE_BUILD_TYPE=Release` | 0 |
| `cmake --build build/block3-fixture --target block3_fixture -j 4` | 0 |
| `cmake --build build/block3-fixture --target block3_measure -j 4` | 0 |
| `build/block3-fixture/block3_fixture sample C build/block3-sample-C-v1` | 0；4张（15°，两圆角条件×无噪声/seed11） |
| `build/block3-fixture/block3_fixture plan H build/block3-plan-H-v1` | 0；240条，每条三工作尺寸，总720定位；未渲染 |
| `build/block3-fixture/block3_fixture generate C build/block3-C-v1` | 0；240张原图及独立实例mask |
| `build/block3-fixture/block3_measure build/block3-C-v1 build/block3-C-measure-v1.jsonl` | 0；720测量，结构失败0 |
| `python3 src/tushenghao/tools/block3_fixture/summarize.py build/block3-C-measure-v1.jsonl src/tushenghao/docs/evidence/block3/C-v1-summary.md` | 0；分层统计候选，非批准生产参数 |

环境：CMake3.28.3、GNU13.3.0、OpenCV4.6.0、nlohmann_json3.11.3、OpenSSL3.0.13。
核心在本地参考包中的摘要见C manifest，参考包不是独立git checkout，不能冒称它已逐文件证明等同远程7daeba2；核查依据是本机源码、核心API和实测结果。

原始C测量约5.8MB，在 `build/block3-C-measure-v1.jsonl`；PNG/真值/manifest在 `build/block3-C-v1/`；统计表在 [block3_budget.md](block3_budget.md)。输出目录禁止覆盖，重跑必须用新版本名；所有失败保留，不剔除分母。

当前缺口：关联recipe的独立依据、assignment正式统计、资源策略/支持仿射范围、全面圆弧收敛测试与H阶段集成。已有工具不能据一次隔离成功证明旧三L分类/拟合全部正确。

## 2026-10-06 实际阶段运行补记

上述H-only-plan、关联未测为早期历史。H240原图已完整渲染于build/block3-H-v1，seed55/66/77/88，中心(1000.25,750.5)，相同24角度×两圆角×五噪声×三工作尺寸=720。正式runner先调用runDecodePipeline，再工具侧读真值评价；未向Detector传真值。C-v6完成720真实三L父投影指标，公式仍复用summarize.py，未把H用于定值。

新增block3_verify用于阶段/隔离核查；--diagnose-stage只读实际父候选/组件指标定位失败，不调门限。正式退出2表示H门槛失败，配置/输入错误退出1。report_verification.py独立核查物理对应、屏幕顺序、bbox和证据，并保存全分母/失败；其退出0只表示报告成功写入。正式H/v2结果689/720、隔离720/720；全视频1676/1676。固定资源8场景及128/4096/64策略完成测量和用户批准。当前未闭合H拓扑/旧三L上游问题与一般支持范围，详见[最终验证](evidence/block3/03-baseline-v1-v2/04-frozen-v2-validation.md)。没有重复造渲染或检测实现；参考包未修改。

## 四项并行修复后的独立复核

新video_corner_measure固定原视频713组，逐角调用正式fitObservedEdgePair且仅解除待求角误差上限，保存全部2852条（不可测null）。后测复用先前真实父矩阵/assignment，避免并行上游改动污染分母；summarize_video_corners复用nearest-rank quantile，用户另授权本轮P99+1方法，不改变原C公式。parent_diagnose全1676逐帧/逐父只读诊断、Ms31/58独立重放均归档。核心渲染库没有改动。

最终C720、H705且原689逐ID保留，视频591Detection；组合raw JSONL与前后713角及各独立H已压缩归档，见[four-track-raw-archives.json](evidence/block3/04-four-track-v3-v4/four-track-raw-archives.json)。[四项结果](evidence/block3/04-four-track-v3-v4/06-four-track-repair.md)明确不可测、设计暂停和正式FAIL，不以诊断分支代替Detection/真值。

## 2026-10-06 三L观察修复最终回归

沿用原C/H全部240原图及manifest，未重渲染、未改旧合成器/模型坐标。冻结v7数值与v4完全一致，显式L观察与候选/原简化轮廓全凹点枚举按用户批准实施。最终C/H stage与isolated均720/720、wrong_valid0，H正式退出0，独立报告H_acceptance=true；原H689和705逐ID保留。视频完整1676/1676退出0，Detection帧864，旧581保留/10竞争保守回退/283新增；无视频真值正确率结论。两个Release完整构建/CTest16/16，新增9项主动观察/锚点/资源/灰度检查。见[实施与命令](evidence/block3/05-L-observation-v5-v7/07-L-observation-repair.md)、[原始压缩双hash](evidence/block3/05-L-observation-v5-v7/L-observation-raw-archives.json)、[适用边界](evidence/block3/05-L-observation-v5-v7/适用边界与已知局限.md)。V仍按用户暂跳，公共process仍NOT_READY，整体WIP，未commit。
