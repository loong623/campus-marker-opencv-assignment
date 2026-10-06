# Block 4：输出稳定层——Codex 完整实现方案（执行终稿）

日期：2026-10-06。用户决策及Oyster架构审查已通过，本文是Codex唯一执行方案；方案定稿不表示代码或验收已经完成。

基线：main `08c5c447348f46b9d033b90f109d48650745b4c6`；目标分支 `feat/temporal-stability`，由用户维护。Codex 不执行 Git 操作，也不通过构建脚本间接执行 Git。

## 1. 结论、依据与开工关卡

本期实现时间感知四点平滑、单目标选择、失检/reset、独立文字桥接和行为审计；**不实施 Detection 置信度，不改变当前帧的检测准入规则。** 六个 lib 模块保留，稳定层作为 pipeline 内部组件落位，不新增第七个 lib 模块。

### 1.1 已逐一读取的依据

| 必读项 | 实际核查 | 用途 |
|---|---|---|
| `src/tushenghao/docs/ref/4.md` | 全文，§4.1～4.10 | 本期公式、状态、桥接、配置、验收的唯一冻结依据 |
| `src/tushenghao/docs/ref/Block3_偏离审计与Codex修复方案_终稿.md` | 全文索引及§3逐项；文件标题仍写“范围批准修订版” | D01～D25 是历史问题编号，不把旧审计判断当现行代码事实 |
| `src/tushenghao/docs/ref/v0.8.1_Detector.md` | 全文 | 原始测量/稳定输出/显示隔离、NOT_READY、公共契约 |
| main 代码与记录 | `lib/`六模块、`include/mark/`、配置、测试、audit、Block3预算/后续修复记录 | 检查实现与证据，而非仅看提交说明 |

固定代码入口：[08c5c44 的 src/tushenghao](https://github.com/shenghaotu81-droid/campus-marker-opencv-assignment/tree/08c5c447348f46b9d033b90f109d48650745b4c6/src/tushenghao)。下文行号均对应该提交，搬迁后的路径以本方案为准。

本轮实际做了源码阅读、历史差异和路径扫描；**没有构建、运行 CTest、重放合成数据或视频**。记录中的 H720/720、视频864/1676来自仓库证据，不是本轮复测；未标注视频的864不能称为正确率。

### 1.2 已批准决策与唯一数值审批关卡

| 项目 | 用户2026-10-06决议 | Codex执行要求 |
|---|---|---|
| G-ABI：选A，已批准 | 给DisplayState追加`std::string value`；明确为ABI例外，FrameResult嵌套布局变化 | 直接实施，不再等待。Detection与FrameResult直接字段、既有公共签名不变；相关目标全部重新构建，不能混用旧ABI二进制 |
| G-C：已确认 | Block4不做置信度，v0.1保持二值输出；v0.2另立“置信度标定”需求 | 生产confidence保持null，不新增评分/软放行；原因四条及七步启用条件必须进入验收文档的“决策记录”节 |
| G-B：实际数值待批准 | correspondence_uncertainty_px、max_smoothing_deviation_px由Codex做实验，用户选定后落地 | 按§6.3测量并提交对照表；生产省略未批准预算、保持NOT_READY，不填经验值。实验fixture与候选配置独立归档 |

G-ABI与G-C不再是阻塞项，不重新请求许可。G-B未批准只阻塞生产就绪及正式稳定验收，不阻塞骨架、显示桥接、主动测试和实验工具。生产不能通过关闭平滑绕过G-B进入正常输出。

六模块保留，稳定层放lib/pipeline；D01～D25的23项处置通过，D23/D25明确范围外，D16按本方案在Block4内修复。这里的“处置通过”不把D16尚未实施的修复说成基线已经修好。

Block3被合入main不代表其所有旧验收项目已闭合：仓库明确V/Q暂跳、模型角边绑定未配置化、部分旧测试Release下assert失效。它们独立记录，不由Block4宣布完成，也不因此把Block4算法的NOT_READY永久绑定到人工标签是否齐全。

## 2. D01～D25技术深审

按已确认审查结论：D23/D25为范围外，其他23项处置通过；其中D16的修复纳入Block4，尚须实施并验收。表中保留基线源码事实，不能用“处置通过”替代代码测试通过。每条同时审中文注释。路径以下以`src/tushenghao/`为根。

| ID | 结论 | 现行源码位置及逻辑证据 | 中文注释与处理 |
|---|---|---|---|
| D01 | 通过 | `lib/corners/corner_resolver.cpp:31–50`按component ID查找，拒重复身份/一片多身份/悬空引用；M缺assignment直接失败。`lib/geometry/geometry_assignment_completion.cpp:12–96`先验证/补全，不按最近中心兜底 | 文件首及35附近说明删除fallback；本期锁定 |
| D02 | 通过 | `lib/preprocess/preprocess.cpp:31–35`保存原图浅视图；`lib/core/prepared_frame.hpp:39–60`像素中心映射；`lib/corners/corner_observation.cpp:10–18`重新从原图分割 | 注释说明低分辨率不能恢复原图证据、不得改输入像素；锁定 |
| D03 | 通过 | `lib/corners/corner_observation.cpp:15–32`完整原图轮廓唯一对应；`lib/corners/corner_edge_fit.cpp:46–111`RDP只标真实轮廓索引，支持来自原连续弧，不闭合裁剩的ROI片段 | 首部及候选枚举注释解释造边与细碎弧问题；锁定 |
| D04 | 通过 | `lib/corners/corner_edge_fit.cpp:30–34,92–168`方向/有限位置双约束、支持不重用、连接弧/凸转折联合验证，不独立挑两条最近方向线 | 注释说明外/内平行边与重复支持；锁定 |
| D05 | 通过 | `lib/geometry/geometry_observation.cpp:69–73`及`lib/corners/corner_observation.cpp:17–18`使用CHAIN_APPROX_NONE；`lib/corners/corner_edge_fit.cpp:14–27`去重点数/跨度；`lib/core/corner_budget.hpp:16–20`最少3点 | 拟合处说明连续弧与去重；正式YAML为10点，不为测试降到2 |
| D06 | 通过 | 原固定ROI半径已消失；`lib/corners/corner_observation.cpp:19–28`用显式原图映射预算，`lib/corners/corner_edge_fit.cpp:31–34`用有限模型边的位置预算 | 改为全原图轮廓搜证是明确实现变化，有性能成本，不能说仍在做局部ROI；本期不优化它 |
| D07 | 通过 | `lib/core/observed_geometry_utils.hpp:35–39`无向锐夹角；`lib/corners/corner_edge_fit.cpp:144–156`夹角、连接弧距离和有限延伸分别检查 | 反平行被折叠为小角；指标不再以线残差冒充角误差；锁定 |
| D08 | 通过 | `lib/corners/corner_resolver.cpp:17–26,60–65`检查映射与投影；最终线/点直接由原图拟合，不再把工作线方向原样搬来 | 注释解释映射/坐标域；锁定。只检查正行列式不等于已有一般仿射条件数保证 |
| D09 | 通过 | `lib/pipeline/decode_stage.cpp:42`给最终validator传`frame.original_image_.size()` | 文件首说明旧size错误；锁定 |
| D10 | 通过 | `lib/pipeline/decode_stage.cpp:68`仅orientation_unique时写方向；`lib/corners/semantic_resolver.cpp:516–529`截断压为未知；decode:48–51对未排除竞争保守拒绝 | 注释说明不能把失败竞争扔掉后声称唯一；118帧不由Block4软放行 |
| D11 | 通过 | `lib/geometry/geometry_assignment_completion.cpp:40`、`lib/corners/corner_resolver.cpp:28–30`、`lib/pipeline/decode_stage.cpp:28`都拒明显不完整；原图触边必要片在observation:37–39拒绝 | 早期排除+最终证据有效性已形成；不是所有bbox触边都直接负例 |
| D12 | 通过（有防御缺口） | `lib/corners/detection_validator.cpp:456–485`复核画内证据/帧/组件；`lib/corners/corner_evidence_validation.cpp:10–75`复算残差/延伸/连接距离并检查交点位于两线，不再忽略config | 注释说明“只看凸点”不够。它没有重新cv::fitLine核验给定线确为L2最优线，也没有图像入参复验灰度来源；当前主链由resolver提供证据。这是追加防御风险，不冒称历史全部安全性质已证明；Block4禁止伪造证据绕过它 |
| D13 | 通过 | `lib/corners/corner_edge_fit.cpp:157–168`填每线支持/残差/延伸/连接弧；resolver:69–76填当前帧、组件、模型边与稳定ID；`tools/audit/decode_audit.cpp:42–64`输出原始载荷 | 支持与ID来源说明清楚；新稳定日志不能覆盖原始测量证据 |
| D14 | 通过 | `lib/corners/semantic_resolver.cpp:334–373`等残差按四角稳定观测ID选已有测量，不平均候选；`tests/block3_contract_regression_test.cpp:103–115`重排检验 | 注释说明测量择优不等于方向胜出；锁定 |
| D15 | 通过 | `lib/corners/screen_order.cpp:531–537,649–675`先finite，再全局最小能量确定容差平局集合和字典序；regression_test:63–86有近菱形平局/NaN | 原始物理编号保留；本期仅允许§4中纯几何排序共享抽取，不改比较规则 |
| D16 | 处置通过；Block4内修复 | `lib/pipeline/detector.cpp:63–85`值初始化/元数据/NOT_READY门控已修；但75–79非法序列直接return，没有clear last_input。未来三类历史若这样处理会存活 | 已说明旧造track问题；必须在非法输入/序列时reset所有状态并INVALID_INPUT，不能只清角或只退回错误；§7落实 |
| D17 | 通过 | `lib/pipeline/decode_stage.cpp:57–67`当前四点min/max浮点bbox，不增加一个像素；confidence/编码保持空 | 注释解释浮点bbox；稳定bbox另重算，不独立滤框 |
| D18 | 通过（状态标签待改） | `tools/audit/decode_audit.cpp:24–85`用唯一runDecodePipeline，值初始化时间/来源，逐帧证据/原因/hash齐；但26、80的public_status=NOT_READY是硬编码 | 现阶段标签正确；Block4后改为NOT_EVALUATED，不能在阶段工具没调用公共process时伪报公共状态。只改元数据标签，历史证据不追改 |
| D19 | 通过（有限覆盖） | `tests/corner_resolver_test.cpp:17–59`真实像素、真角误差、黑原图/缺M/断边/缩放/输入不变；`tests/corner_edge_fit_test.cpp:18–98`圆角、真近平行、远交点及真实连续弧 | 中文说明与主动check存在；不是原“全黑图成功”。旧其他assert测试的Release缺口仍在D20/风险表披露 |
| D20 | 通过（旧其他测试不足） | `tests/detector_contract_test.cpp:82–102`不再丢结果，检查NOT_READY/metadata/空载荷；regression_test:40–62主动检查非法输入/序列 | 公共流程就绪后需替换有完整预算夹具的旧NOT_READY期望；保留缺预算必须NOT_READY，不删负例。`config_contract_test.cpp:1–4`仍空哨兵，不计实质覆盖 |
| D21 | 通过 | `lib/config/config.cpp:159,641–709`finite和范围；`lib/core/corner_budget.hpp:9–20`原图预算防御；semantic_resolver:160–171零预算用≤；regression_test:148–184逐字段非法/往返 | 修复理由明确；Block4新增字段也需相同严格加载/校验/导出 |
| D22 | 通过 | `lib/geometry/geometry_assignment_completion.cpp:12–96`真边界/拓扑/相对面积/方向、多分支、一对一、资源截断；`lib/core/observed_geometry_utils.hpp:12–15`排镜像；decode:89接补全 | 已不再只有三L输入；不得把补全残差当概率或删掉竞争 |
| D23 | 范围外；批准保留 | `lib/geometry/geometry_observation.cpp:218–224`显式L拓扑替代6/7顶点判L；`lib/geometry/geometry_l_topology.cpp:9–43`真实长臂候选。但`lib/geometry/geometry_validation.cpp:224–274`仍是投影/观测面积比，不是三L中位面积分类 | M/S补全用中位面积，原三L面积规则按后续批准保留，不能将D23整体勾成已清零。同平行边族的精确比例有仿射依据，但当前有限栅格验证未覆盖一般仿射误差；见风险。本期不重写Block2 |
| D24 | 通过（有覆盖限制） | `config/detector.yaml:30–93`与`docs/block3_budget.md:94–138`/冻结v7记录有模型确认、C统计与后续审批；实拍角弧4.131595…是后续明确授权例外，不是原3σ结果 | 部分旧“占位/待批”注释保留历史，不能据此说现在未批；新时序预算不得自动复用这个弧距或2px真值测试预算 |
| D25 | 范围外；绑定欠账保留 | `CMakeLists.txt:1–67`无GTest；`lib/corners/corner_resolver.cpp:41–43`、`corner_evidence_validation.cpp:15–19`仍硬编码两份角边表 | 有图纸绑定解释但缺模型配置化/同源表，文档已承认；本期锁定，不新增第三份。需另立模型绑定修复，不顺手改 |

### 2.1 范围与路径扫描结论

确实修改过`geometry_matcher.cpp`及`geometry_validation.cpp`。对比99bc72c：matcher新增源ID查找、全部实测凹点元组、资源计数和非镜像矩阵检查；validator仅改ID查找、诊断传递及include路径，原残差5与面积0.5～2判据未改。`docs/evidence/block3/05-L-observation-v5-v7/07-L-observation-repair.md:5–13`记录这些后续获批范围。本次用户已确认该历史修复范围；Block4继续锁定这些代码，不再追问批准。

路径扫描覆盖lib/include/app/tests/tools的C++与CMake。未发现生效的`/home/...`、WSL UNC、Windows盘符硬编码；`tests/detector_contract_test.cpp:26`有已注释旧/home路径，历史工具日志也有旧机器路径，二者不是执行依赖。CMake用当前源码目录生成默认配置绝对路径，测试用`__FILE__`推导；这是构建位置依赖，不等于某个人/home依赖，运行已编译程序搬迁的可移植性不由此保证。

另有当前`CMakeLists.txt:74–75`的`execute_process(COMMAND git rev-parse HEAD ...)`：普通构建会间接执行Git。按本次Codex约束改为外部传入提交标签，见§4、§8；代码hash仍计算真实源文件。

## 3. 置信度决策：本期不做

**用户已确认：Block4不计算或启用Detection.confidence，生产继续null；v0.1保持二值输出。v0.2另立“置信度标定”需求，经完整流程批准后再启用。** 不给null改填0/1，不给残差改名概率，不将quality_flags当可比较置信分数。

理由：

1. `decode_stage.cpp:48–51`在已取到有效角的同时，仍有不能排除的竞争解释，因此返回NOT_DETECTED。稳定层看不到一个可合法发布的Detection；增confidence不是解决身份/证据缺失的办法。
2. 没有用户确认的V/N/U/O与人工角真值，无法标定“正确几何/正确方向”概率。H720证明固定合成支持条件，不是实拍概率标定集。
3. 冻结规定不可信empty、同几何异方向unknown、历史不补点/方向。用低分发布被硬拒绝的118帧是准入规则变更，超出输出稳定层。
4. 当前单track最大面积/单一关联路径不需要置信评分。本期平局按稳定候选顺序，不为排序捏造分数。

v0.2启用条件按以下七步独立执行，本期只记录、不实现：

1. **用户盲标**：在不看待评估检测输出的情况下标正例、负例、歧义与可测角点，保留标注不确定性。
2. **定义事件**：明确置信度代表何事，例如完整四点是否都在批准定位预算内；方向正确另定义，不混作一个含糊“质量”。
3. **拆分数据集**：按视频/片段分训练或拟合集、校准集、独立测试集，不随机打散相邻帧伪独立。
4. **确定公式**：明确单帧特征、函数、输出含义及失败值；先审公式和所在层，再实现，不将残差直接命名概率。
5. **标定**：只在标定数据上拟合参数及概率映射，保存数据版本/参数来源。
6. **验证**：在独立测试集报告可靠性曲线、Brier分数、覆盖/拒绝率、几何与方向错误，披露失效条件。
7. **审批门槛**：用户批准指标、启用范围与低分处理后才启用；竞争路径准入如需变更另作明确决议。

将来评分只在**当前decode通过硬有效性后**计算，不放在temporal里用历史抬分。本期不选公式、不加未来评分函数或新参数。以上四条原因、七步启用条件必须完整写入`src/tushenghao/docs/block4_acceptance.md`的“决策记录”节。

## 4. 架构影响与精确文件落位

六模块仍为`core/config/preprocess/geometry/corners/pipeline`；pipeline拥有生命周期和稳定装配，corners拥有几何屏幕排序，core只放共享帧元数据，不建utils。

### 4.1 新文件

| 完整仓库路径 | 责任 |
|---|---|
| `src/tushenghao/lib/core/frame_stamp.hpp` | 无图像的FrameStamp值类型，供稳定/显示/audit共享 |
| `src/tushenghao/lib/pipeline/frame_sequence.hpp`、`src/tushenghao/lib/pipeline/frame_sequence.cpp` | 纯元数据合法性检查；不保存全局状态，不执行视频读取 |
| `src/tushenghao/lib/pipeline/temporal_types.hpp` | 关联、对应、三类状态载荷、诊断；落实4.md的temporal_types.hpp |
| `src/tushenghao/lib/pipeline/temporal_stabilizer.hpp`、`src/tushenghao/lib/pipeline/temporal_stabilizer.cpp` | 当前候选选择、时间平滑、失检及实例状态 |
| `src/tushenghao/lib/pipeline/temporal_geometry.hpp`、`src/tushenghao/lib/pipeline/temporal_geometry.cpp` | 稳定点几何/当前偏离检查、bbox重算；不做观测拟合或身份解码 |
| `src/tushenghao/lib/pipeline/display_history.hpp`、`src/tushenghao/lib/pipeline/display_history.cpp` | 独立文字桥接，无角点输入/缓存 |
| `src/tushenghao/lib/pipeline/stabilize_stage.hpp`、`src/tushenghao/lib/pipeline/stabilize_stage.cpp` | 将decode当前测量与稳定输出装配成FrameResult；process和audit共享 |
| `src/tushenghao/lib/corners/screen_order.hpp` | 新增只排序有序几何环的内部接口，不把未知角的屏幕下标当物理P编号 |
| `src/tushenghao/tests/temporal_stabilizer_test.cpp` | §9的时间/选择/对应/失检/reset主动断言 |
| `src/tushenghao/tests/display_history_test.cpp` | §9独立合成文字边界测试 |
| `src/tushenghao/tests/temporal_integration_test.cpp` | 阶段装配及真实Detector公共状态测试 |
| `src/tushenghao/test_support/temporal_fixture.hpp` | 合成可信Detection/文本/帧戳及预期；不包含生产阈值默认值 |
| `src/tushenghao/tools/audit/temporal_audit.cpp` | 一次decode后记录原始/稳定对照；落实冻结tools/temporal_audit.cpp，按现有工具归档规则落位 |
| `src/tushenghao/tools/common/temporal_record.hpp` | temporal_audit专用记录格式，无全局诊断框架/新依赖 |
| `src/tushenghao/docs/block4_acceptance.md` | 逐条完成证据、关卡、实际耗时及剩余项 |
| `src/tushenghao/docs/block4_budget.md` | G-B口径、固定实验与用户批准数值；未批留空 |

### 4.2 可修改文件及修改范围

| 完整仓库路径 | 允许改动 |
|---|---|
| `src/tushenghao/lib/pipeline/detector.cpp` | Impl增加实例组件/就绪判定；process接稳定装配；非法输入/reset清全部状态 |
| `src/tushenghao/include/mark/detector.hpp` | 只更新过期说明；四个调用接口及现有特殊成员签名不变 |
| `src/tushenghao/include/mark/detector_types.hpp` | 按已批准G-ABI给DisplayState追加value及ABI例外中文说明；Detection、FrameResult、TrackResult、枚举/编码定义不动 |
| `src/tushenghao/include/mark/detector_config.hpp` | TemporalConfig补冻结数值/门限及可选对应/偏离预算；追加DisplayHistoryConfig内部配置转换所需字段或类型；不改旧geometry/corner参数 |
| `src/tushenghao/lib/config/config.cpp` | temporal字段名单/加载/校验/导出；分拆“未实现”开关，移除已实现稳定/文字桥接的禁用检查，保留Block5功能禁用检查 |
| `src/tushenghao/config/detector.yaml` | 增冻结temporal参数，完成后稳定默认1；G-B批准前预算节点省略；geometry/corner/assignment全部数值不改 |
| `src/tushenghao/lib/corners/screen_order.cpp` | 仅抽取已有几何排序核心供新内部接口共用；orderScreenCorners保持P0～P3入口及原行为，不改能量/平局/有限性规则 |
| `src/tushenghao/CMakeLists.txt` | 注册源/三个普通CTest/工具；替换间接git元数据；digest包含新增源/头及temporal_audit |
| `src/tushenghao/tests/block3_contract_regression_test.cpp`、`src/tushenghao/tests/detector_contract_test.cpp` | 只替换已完整就绪fixture的NOT_READY断言，保留缺预算/非法输入覆盖，增加非法后状态清除检查 |
| `src/tushenghao/tests/config_error_test.cpp`、`src/tushenghao/tests/config_roundtrip_test.cpp` | temporal新增字段负例/往返；旧geometry/corner负例保留；未实现开关负例仍测Block5 |
| `src/tushenghao/tests/screen_order_test.cpp` | 新共享排序与旧入口等价验证，原用例不删 |
| `src/tushenghao/tools/audit/decode_audit.cpp` | 仅将硬编码public_status改NOT_EVALUATED及对应摘要文字；不改变decode路径或其几何/证据输出 |
| `src/tushenghao/README.md`、`src/tushenghao/docs/INDEX.md`、`src/tushenghao/tools/INDEX.md` | 用法、文件导航、参数单位/审批、已知限制与新工具索引 |

原冻结detector_config.hpp/cpp对应现有“公开类型头+lib/config/config.cpp”；不为满足旧名字另建重复配置加载器。`lib/pipeline/decode_stage.hpp/cpp`不修改：稳定在decode之后接入，不把有状态过滤塞进decode。

### 4.3 不改公共签名；内部签名到此为止

下列为签名/关键类型，非函数体。全用C++17，namespace mark。

```cpp
// lib/core/frame_stamp.hpp：time_source沿用唯一合法Unknown，不增加枚举。
struct FrameStamp {
    uint64_t frame_id{};
    int64_t timestamp_us{};
    TimestampSource time_source{TimestampSource::Unknown};
};

// lib/pipeline/frame_sequence.hpp：当前来源非法/负时间/帧号重复倒退/时间倒退失败；同时间合法。
bool validateFrameStamp(const FrameStamp& current,
                       const std::optional<FrameStamp>& previous,
                       std::string& reason);

// lib/pipeline/temporal_types.hpp
struct AssociationResult {
    std::optional<size_t> current_index;
    bool matched_history{false}, ambiguous{false};
    std::string reason;
};
struct CornerCorrespondence {
    std::array<size_t, 4> current_to_previous{};
    bool valid{false};
    std::string reason;
};
struct SemanticDisplaySample {
    std::string value;
    uint64_t source_frame_id{};
    int64_t source_timestamp_us{};
};
struct TemporalDiagnostics {
    std::optional<double> dt_seconds, alpha;
    AssociationResult association;
    CornerCorrespondence correspondence;
    bool used_smoothing{false}, fell_back{false};
    std::string reset_or_fallback_reason;
};
struct TemporalResult {
    Status status{Status::NOT_READY};
    std::vector<TrackResult> tracks;
    TemporalDiagnostics diagnostics;
};

// lib/pipeline/temporal_stabilizer.hpp：TemporalConfig扩充在公开配置头。
double computeTimeConstantSeconds(double reference_dt_ms, double reference_alpha);
double computeCurrentWeight(double dt_seconds, double tau_seconds);
class TemporalStabilizer {
public:
    explicit TemporalStabilizer(TemporalConfig config);
    TemporalResult update(const std::vector<Detection>& detections,
                          const FrameStamp& stamp, cv::Size original_size);
    void reset(ResetReason reason) noexcept;
};

// lib/pipeline/display_history.hpp：已批准的DisplayState.value承载文字。
struct DisplayHistoryConfig { bool enabled{false}; int max_hold_frames{5}; };
class DisplayHistory {
public:
    explicit DisplayHistory(DisplayHistoryConfig config);
    std::optional<DisplayState> update(
        const std::optional<SemanticDisplaySample>& current, const FrameStamp& stamp);
    void reset(ResetReason reason) noexcept;
};

// lib/corners/screen_order.hpp：数组下标只是几何环slot，不宣称物理身份。
struct ScreenCycleOrder {
    std::array<cv::Point2d,4> screen_points;
    std::array<int,4> input_to_screen;
    bool tie{false};
};
std::optional<ScreenCycleOrder> orderScreenCycle(
    const std::array<cv::Point2d,4>& cycle, const CornerConfig& config,
    std::string& reason);

// lib/pipeline/temporal_geometry.hpp
CornerCorrespondence resolveCornerCorrespondence(const Detection& current,
                                                 const Detection& previous_raw,
                                                 const TemporalConfig& config);
bool validateStableCorners(const std::array<cv::Point2d,4>& stable_by_current_slot,
                           const Detection& current, cv::Size original_size,
                           double max_deviation_px, std::string& reason);
cv::Rect2f boundingBoxFromCorners(const std::array<cv::Point2f,4>& screen_corners);

// lib/pipeline/stabilize_stage.hpp：无第二次decode，生产显示语义源恒空。
FrameResult finalizeDecodedFrame(const DecodeStageResult& decoded,
                                const FrameStamp& stamp, cv::Size original_size,
                                TemporalStabilizer& temporal, DisplayHistory& display);
```

类的私有载荷按§5实现，不用static；TemporalStabilizer私有保存配置、构造计算的tau、选择参考、平滑历史、最近有效输入帧戳/图像尺寸；DisplayHistory私有保存显示源、缺失调用次数、最近有效帧戳。不缓存cv::Mat、模型/轮廓、预测点，不追加track_id。

公开DisplayState补丁固定为：原`source_frame_id`、`age`、`is_held`字段保持名称、类型和顺序，在其后追加`std::string value`。不将value塞入diagnostics，不改FrameResult的display_state字段类型。此补丁已批准；验收必须记录其ABI影响并重新构建所有相关目标。

## 5. 稳定层确定行为

### 5.1 数值规则

构造时只由参考间隔/权重算tau：

`tau_seconds = -(reference_dt_ms / 1000.0) / log1p(-reference_alpha)`。

更新时`dt_seconds = double(current.timestamp_us - previous.timestamp_us) / 1e6`；`alpha = -expm1(-dt_seconds / tau_seconds)`；`stable = alpha * current + (1-alpha) * previous_stable`。内部用double，最终转Point2f后再次检查退化/画内/bbox一致性。不保存第三个可编辑tau参数。

冻结14ms/0.7对应tau约0.011628s；7/14/28ms权重分别约0.452277/0.7/0.91。Δt=0采用当前原始点并重建平滑历史，**不调用公式后用alpha=0冻结旧点**。无历史、失检恢复、关联失败/歧义、对应不唯一、超期、非法平滑都回退当前原始值；审计alpha在未应用时为null，原因明确。

### 5.2 三类状态与独立生命周期

| 状态 | 保存内容 | 何时更新/清空 |
|---|---|---|
| 选择参考 | 上次选中的**原始**Detection、帧戳/原图尺寸 | 有效选中时更新；empty不更新时间；当前时间距参考>50ms过期；reset/非法/换源清 |
| 平滑历史 | 上次稳定点按“上次原始屏幕slot”索引、上次原始点/可信orientation、帧戳 | 连续可信当前测量更新；empty立即清；恢复首帧/回退重新建立；reset/非法/换源清 |
| 显示历史 | 文字、来源帧/时间、连续缺失调用次数 | 只由DisplayHistory规则更新；不读取有效点或orientation |

保存稳定点的内部slot是在当前原始屏幕环下的索引，**不受最后稳定屏幕排序起点影响**。否则屏幕分界刚换位后，下一帧又会错混角。内部slot映射与对外LT/RT/RB/LB排序分别记录。

50ms边界：间隔≤50ms允许使用选择参考，>50ms清参考及平滑。参考时间只在有效选中时推进，不能靠连续empty无限续命。双实例状态完全隔离，重复reset安全且noexcept。

### 5.3 候选选择与关联

1. 输入约定为当前decode已验证Detection集合；update再做原图size正值、finite/凸性/原图界限/方向映射格式及屏幕排序的防御检查。共享排序重排后点序必须与输入相同，不能接受任意循环起点冒充LT；异常候选集合返回INVALID_INPUT并清状态，不静默丢一个候选后宣称唯一。
2. 无参考或过期：按四点shoelace绝对面积选最大，完全等面积按输入顺序。不用bbox面积代替多边形面积，不用confidence/quality_flags排序；本期未定义可比质量。
3. 有参考：中心=四点均值；D=参考原始bbox对角线，要求D>0。候选中心距≤0.5D、面积/参考面积在[0.5,2]为可能关联，两个边界均包含。
4. 恰好一个可能关联：选它，matched_history=true。零个：重选全体最大面积、关联失败、原始透传并重建平滑。多个：本期没有经批准的进一步区分依据，全部视为歧义，同样重选全体最大面积并重置；不补一个nearest/分差阈值。
5. 即使输入有多个Detection，FrameResult.detections仍保留全体当前测量；tracks最多一个且detection_index引用其中有效位置。不实现多目标轨迹或持久ID。

### 5.4 四点对应先于平滑

两帧orientation均可信且是0～3双射：物理p对应当前屏幕`o_cur[p]`和上帧原始屏幕`o_prev[p]`，得到current_to_previous。不能将后排序的稳定屏幕索引误用为上帧原始索引。

任一方向未知：只枚举四种**同绕向循环**`m_k(i)=(i+k)%4`，不枚举反射、不推定物理身份。按§6的归一化位移与误差区间判唯一；不唯一用当前原始值并重建平滑，不把历史orientation写入当前属性。

对对应后的四点执行一阶滤波，先在当前slot下验证，再用共享纯几何orderScreenCycle排屏幕顺序。未知方向不能假装slot是P0～P3调用物理接口；已知当前方向则新方向`o_out[p] = input_to_screen[o_cur[p]]`。当前orientation为空时稳定orientation必须空，哪怕上一帧已知。

稳定结果从当前Detection复制，仅替换稳定corners、重算bbox、按当前可信方向更新映射；marker_code/confidence/category/quality_flags不得由历史生成。FrameResult.detections及decode测量/证据完全不改。

### 5.5 稳定几何检查与失败回退

检查四点finite、不重复、正面积凸环、非自交、在当前原图`[0,width)×[0,height)`、对应后的每角距当前点≤批准max_smoothing_deviation_px；转float后重复检查及bbox min/max。容差只用机器舍入一致性，不加“画外裁回”等工程宽松量。

稳定点通常不等于原始拟合交点，因此**不伪造CornerMeasurement/CornerEvidence交给Block3最终validator**。新函数只检查稳定几何与当前偏离；原始证据在decode已经验证。稳定失败输出当前原始Detection的track，原因可查；重建平滑历史，不把可信当前检测删掉或报NOT_DETECTED。

### 5.6 失检、非法、换源

- empty：有效Detection及track立即空；平滑立即无效；选择参考在未超期时可保留。恢复首帧即使关联成功也必须原始值，下一连续有效帧才滤波。
- 重复/倒退frame_id、时间倒退、负时间或非法来源：INVALID_INPUT；清选择/平滑/显示及输入序列状态，当前不产生输出。下一合法帧可重新开始。
- 图像格式非法：公共入口先reset(InvalidSequence)再INVALID_INPUT，不进入decode。
- 原图尺寸变化：视作InputChanged，清全部历史；当前合法帧作为新段原始首帧处理，记录原因。
- 输入源的身份字段不存在，不能自动知道两段同尺寸视频来自不同源。应用/工具换源或循环前必须`Detector::reset(InputChanged)`；不新增source_id/时间枚举，不用识别画面代替它。

DisplayHistory自己的有效帧号/时间校验同样执行；有效关闭时当前文字透传，不存桥接历史，仍可保存校验用上次帧戳。非法时清空并返回null。

### 5.7 桥接精确行为

生产finalizeDecodedFrame恒传`std::nullopt`给DisplayHistory；不从orientation、编码草稿、diagnostics生成文字。A/B只用于合成测试，绝不能造MarkerCode。

启用时，新文字来源帧/时间必须等于当前stamp；新值输出held=false、age=0并替换历史。缺失按**有效update调用次数**加1：第1～5保留原文字/来源、age=1～5、held=true，第6清空且丢历史，后续仍空。跳过帧号不补调用。max_hold_frames=0首个缺失就清。

关闭时新值只透传、本次无值即null，没有桥接源缓存。reset、非法戳、换源立即清，不把清状态动作计作一个缺失帧。DisplayHistory不接受可写Detection或track，测试必须证明桥接不会改有效输出。

## 6. 配置、误差预算与批准办法

### 6.1 新增/保留参数

| YAML完整键 | C++字段 | 值/校验 | 来源与状态 |
|---|---|---|---|
| temporal.stabilization_enabled | 同名bool | 完成后默认1；0只关闭平滑，不取消当前选择/track | 4.md§4.8 |
| temporal.reference_dt_ms | 同名double | 14；finite且>0 | 冻结 |
| temporal.reference_alpha | 同名double | 0.7；finite且严格(0,1) | 冻结 |
| temporal.history_max_gap_ms | 同名double | 50；finite且>0 | 冻结 |
| temporal.max_center_distance_diagonal_ratio | 同名double | 0.5；finite且>0 | 冻结“参考目标对角线0.5倍”落实字段名 |
| temporal.min_area_ratio / max_area_ratio | 同名double | 0.5/2.0；finite、0<min≤1≤max | 冻结 |
| temporal.correspondence_uncertainty_px | 同名optional<double> | 缺失表示未批；有值finite且≥0，原图px | G-B新预算，值待批 |
| temporal.max_smoothing_deviation_px | 同名optional<double> | 缺失表示未批；有值finite且>0，原图px | G-B新预算，值待批；不能拿0关闭平滑验收 |
| temporal.display_hold_enabled | 同名bool | 0；0/1类型严格 | 冻结 |
| temporal.max_hold_frames | 同名int | 5；非负整数，拒浮点/溢出 | 冻结 |
| output.show_held_state | 原字段 | 保持0，本期不实现绘制开关 | 冻结/Block5边界 |

可选数值不写`""`、0或经验值冒充留空；未批准时省略对应键，文档数值栏写“未批准”。存在但类型非法仍报ConfigError。生产加载允许缺预算用于NOT_READY审计；TemporalStabilizer开启平滑构造时要求两个预算齐全，缺失不得静默关闭。Detector仅在其准备齐全时构造可运行的稳定组件。

旧schema_version=1配置兼容规则明确为：原有temporal三个键仍必填；本次新增且已有冻结起点的reference_dt_ms/reference_alpha/history_max_gap_ms/中心门限/面积比六个键可省略，分别采用上表**已批准冻结值**，存在时严格校验。有效配置导出写全这些值；缺两个未批准预算绝不补默认。这样旧冻结v7及现有测试配置可加载，不改旧快照。新增TemporalConfig默认值和生产新YAML一致；stabilization_enabled程序默认值在实现完成时改true，旧YAML明确0仍尊重0。不增schema版本或新的DetectorMode。

内部TemporalStabilizer的关闭平滑fixture可不使用两项平滑预算，仍选当前原始候选并返回单track；这只用于机制测试。**生产G-B未批准或缺失时，无论平滑开关为何值，公共流程均NOT_READY。** G-B获批落地后，显式关闭模式才可正常透传；正式默认开启、正式验收必须开启。增加字段同步checkFields、缺失策略、load、validate、writeEffectiveConfig及测试；不添加热加载。

### 6.2 循环对应的确定公式

给定当前原始环z、上次原始环q，参考原始bbox对角线D>0；r为批准的单帧每角位置不确定度。对四种循环k：

`d_i(k)=||z_i-q_(i+k)||`，`E_k=Σd_i(k)^2 / D^2`。

两次测量误差相加，距离区间取`[max(0,d_i-2r), d_i+2r]`，得`L_k=Σmax(0,d_i-2r)^2/D²`、`U_k=Σ(d_i+2r)^2/D²`。只有一个k满足`U_k < min_(j≠k)L_j`才采用；区间重叠或机器精度内贴边视为歧义。机器精度比较采用现有64×epsilon×量级规则，不能另添视频分差门限。

这是保守误差区间，非置信概率；D仅是共同正归一化分母，不据其估计真值。用**原始对原始**判对应，然后用保存在原始slot下的稳定历史做滤波，避免把滤波滞后当对应误差。

已有`max_corner_error=4.131595…`是交点到连接弧距离，`semantic_geometry_threshold=64`是四角集合平方和，合成真值2px只是特定条件下验收限；三者都不是自动批准的每帧r，也不是允许时序滞后的预算。

### 6.3 G-B实验与提交流程（不视频拟合）

**状态：两个生产数值均待批准。Codex负责测量和整理，用户负责选择；不能因一组实验看起来更顺滑就自行填写生产。**

#### A. 测correspondence_uncertainty_px

1. 复用现有C/H连续物理角真值与实际定位误差记录，不重新生成海量网格。先核对模型、配置、输入、源码hash及四角物理对应。缺原始记录时使用现有runner补测；不从视频估计真值。
2. 每样例记录四个原始角点的欧氏真值误差，并取四角最大值；按工作尺寸×噪声×圆角分层。失败保留原因和分母，不当作零误差或静默剔除。
3. 用C形成r候选：各层`max(均值+3×样本标准差,P99)`，原图px向上取0.5px，再取各层最大值。标准差用n−1，P99用nearest-rank；结构失败或层内样本不足时先报告，不伪造候选。
4. H用于独立交叉检查候选支持范围，不根据H或视频结果倒调r。超出候选范围的比例、最大值和失败原因如实报告；2px历史真值验收限只作对照，不自动成为r。
5. 用T09/T10/T10b两帧/三帧真值语义fixture检查：选出的对应是否正确、歧义是否回退、已知方向是否按物理编号。保留“错对应、唯一对应、歧义回退”的数量，不能只给平均距离。

定位统计表必填：

| 数据集/层 | 条件与hash | 总样本/可测/失败 | 均值 | 样本标准差 | P95/P99/最大值 | 公式原值/取整候选 | H交叉检查 | 支持范围/缺口 |
|---|---|---|---|---|---|---|---|---|
| Codex填实际结果 | 实际记录 | 不改分母 | 原图px | 原图px | 原图px | 原图px | 超限数量与比例 | 距离/仿射/光照等限制 |

#### B. 测平滑收益、偏离与回退次数

max_smoothing_deviation_px是**允许的滞后/相对当前偏离**，不能由定位噪声3σ自动推导。实验使用固定语义序列，直接输入可信Detection，不把合成真值交给正式Detector识别。

保留§9所有断言，另外做有限对照网格：

| 实验因素 | 固定实验输入 | 说明 |
|---|---|---|
| 时间间隔 | 7/14/28ms | 冻结数值锚点 |
| 目标几何 | 100px边长方框，初始中心(720,540)，原图1440×1080 | 几何fixture，不改MARK图纸模型 |
| 每段长度 | 32次有效调用 | 首帧原始；统计保留全段，并注明哪些差分从第二帧开始 |
| 平移 | 每调用0/0.5/1/2/4/8原图px，沿x轴 | 固定实验运动，不是生产关联门限 |
| 旋转 | 每调用0/0.5/2/5°，绕初始中心 | 独立于平移做对照，不做全笛卡尔积；同时测方向已知与未知 |
| 测量噪声 | 无噪声；复用C的已记录角点误差序列作为确定性扰动 | 固定case顺序/ID及扰动，不另造视频拟合噪声；扰动后仍须是合法当前Detection |
| 偏离上限实验档 | 0.5/1/2/4原图px | 仅为对照配置，全部标“实验值，未批准”；不写生产YAML |
| 对应r | A步骤测得候选；r=2px仅保留§9机制测试 | 不把机制测试值冒充测量结论 |

这些数值是预先固定的实验条件及比较档位，不是经验默认值，也不保证用户最终选其中某档。旋转与平移分开跑，复用相同输入进行各档对照；不用扩大到无关成像网格。若扰动导致几何非法，保留该样例/原因并区分输入非法与滤波回退，不偷偷重采样。

对每段记录：

- 原始和稳定对真值的四角RMSE；静止段的raw/stable抖动RMS及比值。
- 各角稳定点相对当前原始点的距离，汇总均值/P95/P99/最大值。
- 运动段沿运动方向的滞后，和已知真值轨迹的RMSE，不能把轨迹运动当静止抖动。
- 实际应用平滑次数、回退次数及原因分项、错对应次数、合法当前测量被误删次数。
- raw是否保持原样、方向unknown是否被历史回填、empty是否产生track。

对照表必填：

| 序列ID/条件 | r实验值 | 偏离上限实验档 | raw/stable真值RMSE | 静止抖动raw/stable/比值 | 当前偏离均值/P95/P99/max | 运动滞后 | 平滑次数 | 回退次数/原因 | 错对应/误删/历史泄漏 |
|---|---|---|---|---|---|---|---|---|---|
| Codex填实际结果 | 标未批准 | 标未批准 | 原图px | 运动段不适用写N/A | 原图px | 有方向与单位 | 实际调用数 | 不合并成“失败” | 各项单列 |

#### C. 提交、用户选择与落地

1. 将统计表、对照表、原始CSV/JSONL、命令/退出码、hash、支持范围和失败样例索引写入`src/tushenghao/docs/block4_budget.md`；大产物放`build/block4-evidence/`，文档登记路径/hash，删build前须归档。
2. 提交一页选择摘要：r候选依据、每偏离档的收益/滞后/回退代价、哪个条件超范围。可以提出推荐组合，但不替用户选择，不宣称“最优即批准”。
3. 用户明确选择两个值及适用范围。若用户要求其他档位，只补该档必要对照，不改算法或倒调视频阈值。
4. 批准后记录原文、日期、两个值、单位、数据/算法版本及范围；同步YAML、强类型加载/校验/导出和测试，重跑相关主动测试与一次正式回归。
5. 未批准时生产预算键省略、公共process保持NOT_READY；实验候选配置独立保存并标EXPERIMENTAL，不替换生产配置。内部fixture可正常验机制，但不据此宣称生产已就绪。

材料不足时写BUDGET_BLOCKED及缺什么；不取864个视频检测帧的分布填值，不挑一个使全部序列顺滑的上限。等待G-B不授权删范围；显示桥接、文档、无依赖测试继续执行。

## 7. decode接入与公共process就绪

### 7.1 固定调用路径（示意，非实现）

```text
Detector::process(FrameInput)
  → 值初始化FrameResult；复制帧号/时间
  → 校验图像与FrameStamp；非法：reset全部 → INVALID_INPUT
  → 已知换源/reset由调用方完成；尺寸变化清历史
  → 检查预算/组件是否就绪；未齐：清历史 → NOT_READY
  → runDecodePipeline(frame, config, marker_geometry)  // 原函数不改
  → finalizeDecodedFrame(decoded, stamp, original_size, temporal, display)
       → decode NOT_READY/INVALID_INPUT：不发布阶段payload，清状态，原状态返回
       → decode DETECTED/NOT_DETECTED：temporal.update(decoded.detections,...)
       → display.update(nullopt, stamp)               // 生产始终无显示语义源
       → detections=原始集合；tracks=稳定集合；追加temporal原因
```

阶段detected与detections非空、not_detected与空必须相符；不符视内部错误，主动报错，不用继续平滑掩盖编排bug。temporal防御返回INVALID_INPUT时，公共载荷清空、三历史reset。合法empty不是非法序列。

### 7.2 NOT_READY退出条件

输入合法、模型/assignment/原图角预算已配置且通过校验、G-B两数值已批准落地、decode实际就绪、稳定组件已实现并按当前模式就绪、显示桥接及已批准DisplayState补丁已实现时，正常返回DETECTED或NOT_DETECTED。不再无条件加STABILIZE_NOT_READY。

G-B未批/缺配置→NOT_READY，既不发布raw Detection也不造track，关闭平滑不能绕过此关卡。两预算获批后，关闭平滑的显式模式可就绪，但不能据此宣告Block4稳定验收完成。decode内部NOT_READY必须原样传播，不能改NOT_DETECTED。

`DetectorMode::Skeleton`是现有唯一枚举和历史配置标签，本期不引入新的mode值；修正“永远空壳”的过期注释，**就绪由真实阶段与预算决定，不靠mode字符串跳过算法**。Block5计时/绘制未实现不阻断默认关闭的算法流水线；相应功能开关仍被严格拒绝。FrameResult没有新增diagnostics/pose等字段，本期只使用已有字符串diagnostics。

### 7.3 reset、审计与源隔离

Detector::reset清last_input、原图尺寸记录、temporal选择/平滑、display源及帧戳；只调reset不重新加载模型/config。输入非法路径调用相同清理函数；不能只把本帧status写错而留缓存。

temporal_audit一次runDecodePipeline后调用同一finalizeDecodedFrame；直接记录该次decode的原始集合和稳定结果，避免为了对照每帧再跑一次约100ms的decode。工具的集成状态注明“共享阶段装配”，公共process等价性由integration_test另验，不能伪称工具另调过process。

decode_audit继续仅验Block3；public_status改NOT_EVALUATED。Block3几何/方向/证据输出必须与基线相同，比较时只排除已声明的public_status标签、源码/配置hash和耗时。旧日志不回写。

## 8. Codex执行顺序与每步产物

开工前检查不计入八个实施步骤。

| 步骤 | 动作与产物 | 验证/停点 |
|---|---|---|
| 开工前 | 阅读本方案、4.md及main参考；用户已在目标分支，保存基线只读证据。记录目录与配置hash，不执行git | 没有同名参考/基线不符，先报告，不对未知结构打补丁 |
| 1 | 新FrameStamp、纯序列检查、temporal类型、状态类骨架；新增三普通CTest目标 | 能编译，三个状态互不缓存，无static历史；仅实施已批准DisplayState字段补丁 |
| 2 | 配置链路与公式；去掉已实现temporal功能的旧禁用条件。CMake用`MARK_COMMIT_LABEL` cache字符串替代git调用，默认UNSPECIFIED | tau/权重锚点、所有非法/缺失/往返；Block5开关仍拒；没有间接Git |
| 3 | 当前选择/历史门限/超期、零时间、失检恢复；写主动check | 多候选不吞当前，empty无track，50ms等号边界、双实例/reset通过 |
| 4 | 对应/滤波/纯几何排序共享抽取/稳定检查；只改允许的screen_order文件 | 旧物理排序逐例同结果、分界不混角、unknown不回填、无效回退；G-B仅fixture值 |
| 5 | 提交G-B测量/滞后表，等用户批准实际值；批准后写生产与有效配置 | 没批不填、不放宽、不声称正式完成；其余无依赖工作继续 |
| 6 | 直接落实已批准显示文字类型，DisplayHistory及A/B边界验收 | 5/6、0、首次缺失、关闭/reset、跳号；不生成任何有效编码/点 |
| 7 | Impl装配与共享finalizeDecodedFrame，非法全清，替换有预算fixture过期NOT_READY断言 | 缺预算仍NOT_READY，正常黑帧NOT_DETECTED，真实合成检测DETECTED，reset/实例隔离 |
| 8 | temporal_audit和记录、两份Block4文档、README/INDEX全部更新；运行一次批准配置全视频对照与现有H阶段非回归 | §9全部强制项通过且文档齐全再写完成；未跑写NOT_RUN，G-B未批写BLOCKED；不删旧失败 |

构建方式沿用现有Linux环境，无新测试框架：

```sh
cmake -S src/tushenghao -B build/block4 -DCMAKE_BUILD_TYPE=Release -DMARK_COMMIT_LABEL=08c5c44
cmake --build build/block4 -j 4
ctest --test-dir build/block4 --output-on-failure
build/block4/temporal_audit --video data/raw/marker_video.avi --config src/tushenghao/config/detector.yaml --output build/block4-evidence/temporal.jsonl
```

`MARK_COMMIT_LABEL`由用户提供，仅是基线标签；实际未提交改动由源hash标识，不能把08c5c44标签当当前工作树精确提交。CMake不加新的必需依赖；OpenCV和现有工具OpenSSL沿用。代码库link边界不改变：公开include，内部lib仅工程内目标显式消费。

默认工具--video/--config/--output均必填，避免个人路径；输出父目录可创建，输出已存在默认报错，显式`--overwrite`才替换本次新产物，不删除旧归档。非零退出码用于输入/配置/记录失败，不能遇到视频open失败还exit0。

## 9. 必测序列与可执行断言

测试统一使用主动check/异常计数、返回非零，Release和Debug都实际执行，不引GTest。以下r/偏离值为明确fixture数据，不能据此写生产。基本方框Q=[(100,100),(200,100),(200,200),(100,200)]，orientation=[0,1,2,3]，原图1440×1080，t单位us；每次调用frame_id严格递增。没有特别说明用批准参考14ms/0.7，历史50ms。

| 测试 | 输入 | 预期与断言 |
|---|---|---|
| T01 公式 | computeTimeConstant(14,.7)，dt=.007/.014/.028 | tau等于公式，alpha约.452277/.7/.91；double误差≤1e-12；不把tau近似常数写入实现 |
| T02 非法参数 | dt参考0/负/NaN/Inf，alpha0/1/NaN，负hold，门限NaN，缺两预算之一 | 非法配置拒；缺预算非空配置按就绪规则，开启update实例不默默降级；往返不丢字段 |
| T03 首帧/连续 | t=0,Q；t=14000,Q+(1,0)，两帧方向可信 | 首帧原始；第二帧稳定Q+(.7,0)，原始集合仍Q+(1,0)，bbox由稳定点重算；float每点误差≤1e-5 |
| T04 实际Δt | 重置后Q，再t=7000或28000,Q+(1,0) | 稳定位移对应.452277或.91，不用固定.7 |
| T05 Δt=0 | Q@0，Q+(1,0)@0，下一帧再移动 | 第二帧原始、alpha=null、ZERO_DT；第三帧使用第二帧重建历史 |
| T06 有效→empty | Q@0，空@14000 | 当前detections和tracks都空，平滑失效，不能出现历史点；未超期选择参考可留 |
| T07 恢复 | Q@0，空@14000，Q+(2,0)@28000，Q+(3,0)@42000 | 恢复帧原始2；下一帧2.7；不从Q跨空段滤成1.4 |
| T08 已知→未知 | 已知Q，再当前orientation=null的平移Q | 当前和稳定orientation均空；几何可以稳定；历史已知不回填 |
| T09 分界换位 | 连续菱形跨排序分界，物理坐标由固定旋转矩阵生成，已知orientation随当前排序置换 | 按同物理角计算golden滤波，输出再次排序；三帧以上验证内部slot历史没有错位，raw始终不改 |
| T10 未知循环 | Q绕中心(150,150)先旋转44°、后46°，分别正确屏幕排序且orientation=null，r fixture=2 | 纯对应helper及update选正确循环；按实际同角轨迹计算golden，稳定点不拉向邻角，输出方向仍null；不能拿未按LT排序的伪Detection测试 |
| T10b 对应歧义 | Q@0后绕(150,150)旋转45°@14000，两帧正确排序、orientation=null，r fixture=2 | 两循环误差区间重叠；原始回退、CORRESPONDENCE_AMBIGUOUS、重建历史；不得选择第一个最小值冒称唯一 |
| T11 关联失败 | Q后输入中心距>0.5D的可信方框 | 当前原始track仍存在，matched_history=false；不删除Detection |
| T12 多候选 | 无历史大小不同/完全等面积；有历史恰1、0、2项落门限 | 大面积/稳定顺序，单track引用合法索引；多个可能匹配则歧义重选，raw集合全保留 |
| T13 门限边界 | 中心距恰0.5D、面积比恰.5或2；历史gap=50000与50001 | 三门限包含等号；gap50000可参考，50001过期原始；不增加epsilon宽松量 |
| T14 平滑非法 | temporal_geometry直接输入非凸/自交/越界/重复/NaN；update用Q@0、Q+(10,0)@14000且偏离上限fixture=2，再Q+(11,0)@28000 | helper明确失败；第二帧候选平滑滞后3px超过2，输出可信当前原始track并重建历史；第三帧输出Q+(10.7,0)，不裁点/不借证据伪通过 |
| T15 时间/帧号非法 | 已有历史后重复id、倒退id、倒退time、负time、非法source枚举 | INVALID_INPUT、三状态清；下一合法低id/新段首帧原始；不能只reset平滑 |
| T16 换源/循环/尺寸 | Q后reset(InputChanged)、frame_id从0；同尺寸换源必须调用reset；尺寸变化 | 新段原始首帧，显示历史清；测试/README明确外部调用责任 |
| T17 实例隔离 | A/B交错喂不同轨迹，其中仅A reset；与各自单独运行比较 | 每帧坐标/方向/原因与单独运行一致，B不受A影响 |
| T18 关闭平滑 | stabilization_enabled=0，连续移动与失检 | 当前选择track原始透传，empty仍空；不应用滤波；正式完成验收另测开启 |
| H01 5/6桥接 | A@frame0，连续6次null，hold=5开启 | A当前held=false/age0；1～5 A原来源/age调用次数；6 null；有效点/track不受影响 |
| H02 首次缺失/零hold | 从未A先null；另A后首个null且hold=0 | 两者null，无伪造文字 |
| H03 新值替换 | A，null，B，null | B立即held=false/age0，新来源；之后只hold B |
| H04 跳号 | A@id0，null@id100，null@id200 | age1、2，不是100、200 |
| H05 关闭/非法/reset | 关闭A→null；开启A后非法stamp/reset | 关闭不保存历史；非法/reset立即null；后续缺失仍null |
| I01 公共状态 | 缺assignment或corner预算；开启缺G-B；完整配置黑帧；完整合成MARK | 分别NOT_READY/NOT_READY/NOT_DETECTED/DETECTED；正常最多1 track，空时0，metadata准确 |
| I02 层间隔离 | 当前empty；或当前unknown而历史已知；显示独立合成测试 | empty无有效点/track，unknown不回填；生产不注入假文字，桥接单测不改有效输出 |
| I03 raw/稳定一致 | 同一真合成帧经runDecodePipeline与process | process.detections字段逐一等于decode；稳定结果仅允许点/bbox/当前方向排序变化；confidence/marker_code仍null |
| I04 原图/当前引用 | 合成目标放原图右下超过工作图边界；多候选fixture选择非0索引 | 坐标不误判工作size，track.detection_index指向本帧正确Detection；稳定bbox逐点min/max |
| I05 公共非法清理 | 先建立Detection/track，再非法图像/序列，最后合法新帧 | INVALID_INPUT且载荷空，新帧原始；不能保留旧显示/位置 |
| I06 旧排序等价 | 原screen_order所有样例与旋转0～345°15°步长、平局/非有限样例 | 共享核心抽取前后P0～P3映射/屏幕点/tie/失败完全一致；未知环新接口只产生slot映射 |

### 9.1 全视频与旧板块非回归

另补生产门控断言：G-B缺失时，即使stabilization_enabled=0，公共process仍NOT_READY。T18只验证内部机制或G-B已批准后的显式关闭模式。

使用同一1676帧视频，0～1675，固定原视频SHA与geometry/corner/assignment冻结v7数值不变；时间来自视频fps生成的单调微秒序列，不用处理墙钟时间算alpha。若实际帧数/hash不同先报告，不强行套分母。

一次temporal_audit同时记录raw与stable，逐帧确认raw状态/四点/bbox/方向/截断/原因与原Block3归档相同（排除已说明的元数据/耗时）；正常864 raw Detection只是**回归计数**，不是新增真值召回门槛。812 empty帧必须有效tracks=0，生产display=null；恢复首帧必原始；每个稳定点偏离与几何检查合格，所有回退带原因，无NaN/崩溃/借历史方向。

如任意raw变化，即使Detection数量相同，也先检查越权改动/输入/配置/工具协议；不能归功于平滑，更不能修改Block3阈值补回来。旧H720阶段回归复用已有图片与runner一次，不重建海量网格；最少保持旧720通过ID及错有效输出0。完整CTest旧16项加新3项全通过；旧“程序数”不代替本节主动断言计数。

### 9.2 审计格式与完成标准

逐帧记录：frame_id/timestamp/time_source/原图size、decode状态和raw集合、track索引/稳定四点/bbox/当前方向、dt/alpha是否应用、关联/对应映射/区间及原因、回退/reset原因、display来源/年龄/held（生产null）、输入/有效配置/模型/源码hash及批准预算版本。数字不存在用null，不写0；文字桥接合成日志单独归档。不写Block5阶段计时/性能达标结论。

**必须达到：**G-ABI/G-C已批准决策落实；G-B两个实际预算获批落地；所有上述主动测试通过；公共就绪/非法/empty隔离通过；raw逐帧非回归与旧H/CTest通过；默认平滑开启和桥接关闭；中文注释/目录/公开契约检查通过；`src/tushenghao/docs/block4_acceptance.md`、`src/tushenghao/docs/block4_budget.md`、README及docs/INDEX.md、tools/INDEX.md更新齐全且有可重放命令。**文档缺失视为未完成；任一必需文档缺失或未更新，不得写“Block4完成”。** 任一其他强制项未齐也为WIP/BLOCKED。

**建议目标：**固定静止噪声序列的稳定抖动小于raw，报告固定运动序列滞后与回退频率；真实视频画面更稳。没有真值与批准指标时不拿“更好看”作硬准入，也不降低上述安全条件。14ms是此前用户系统目标，非Block4的性能验收承诺；当前Block3记录均值约97.65ms、P99约428.37ms，稳定层不能修复这项差距。

## 10. 风险与后续处理

| 风险 | 影响/触发条件 | 本期处理与后续 |
|---|---|---|
| 已批准ABI变化 | DisplayState追加value导致FrameResult嵌套布局变化；混用旧头/旧库可能失配 | 已批准选A，直接实施；库/app/测试/工具全部重建并在决策记录披露，不再等待许可 |
| G-B未批/支持范围有限 | 未知方向对应、滤波滞后无法量化；现有合成2px不能自动推广实拍 | 未批生产一律NOT_READY，不能关闭模式绕过；正式完成必须批准并开启，超范围保守回退 |
| D16非法状态遗留 | 将来历史跨非法/循环污染 | 在本期process/reset与共享序列检查内修，新增恢复反例 |
| D23原三L面积规则 | 与全面相对尺度目标仍有差异，远距离/一般仿射失败 | 按当前后续批准规则保留；另评Block2，禁止本期重写 |
| 一般仿射成像覆盖不足 | 同边族严格平行时dot比例化为同向长度比，精确仿射保持；实际短边/噪声栅格不保证严格平行，有限H旋转/等比无法证明一般仿射检出与误差预算 | 不把几何比例原理当全成像范围保证；另做固定仿射/短边反例与预算评审，本期不改观察器 |
| D25两份硬绑定 | 换模型顶点编号后corner/validator可能不一致 | 固定模型hash与编号，绑定配置化另立任务，不由temporal复制表 |
| D12证据防御不足 | 外部伪造内部证据/给定线非实际最优拟合可能绕过部分检查 | 内部API只信当前生产resolver；新增稳定输出不造证据。更强复拟合/来源校验另审，不声称完全防篡改 |
| 旧assert/空哨兵 | Release绿色不证明那些断言执行 | 新测试全部主动检查；既有16程序只称历史测试通过，遗留覆盖不洗白 |
| 118竞争拒绝 | 历史/评分诱使工程方放行错误解释 | raw empty保持empty；置信度与竞争准入另立v0.2需求 |
| 来源不可识别 | 同尺寸新视频/循环没有source_id | 调用方必须reset(InputChanged)，工具测试覆盖，不新增公共字段 |
| 快速旋转且方向未知 | 四角近对称时最近循环仅是几何对应，不能保证物理身份 | 始终unknown、区间歧义原始回退；不得给持久身份保证 |
| 性能成本 | 原图每角全图重新提取、连续弧组合规模大，超过14ms | 不在Block4优化；audit只decode一次，留Block5测量/后续优化；不能隐藏慢帧 |
| 视频缺人工V/Q | 无法给实拍召回/错方向率/概率标定结论 | 本期只做行为和raw非回归；人工标签按用户另排 |

## 11. 禁止清单、交付与执行提示词

### 11.1 Codex硬边界

- 不改`src/tushenghao/docs/ref/`任何冻结原文，不回写旧evidence、冻结v1～v7配置或v0.1。
- 不改`src/tushenghao/lib/geometry/`全部生产文件，包括geometry_matcher.cpp、geometry_validation.cpp、观察/拓扑/assignment；不改preprocess生产算法/marker_geometry模型。
- corners只允许§4.2的屏幕排序共享抽取；其余corner_resolver/edge_fit/evidence/semantic/validator禁止顺手重构或修阈值。
- 不改Detection/FrameResult直接字段及四个公共调用接口；DisplayState按已批准选A追加std::string value。不能声称其ABI毫无影响，不再等待许可。
- 不增预测、卡尔曼、PnP、多目标匹配、持久ID、有效MarkerCode、置信度或软放行；不把历史文字/点/方向变成当前测量。
- 不新增库/测试框架，不建万能utils或第七个lib模块，不把tool JSON/OpenSSL带进Detector算法库。
- 不通过关闭平滑、填0预算、换模型、降门限、删测试换绿或`detections>=1`宣称完成；不把视频检出数量当真值准确率。
- 不执行git命令/提交/推送/切分支；构建脚本也不间接执行Git。用户处理版本管理。
- 不改未授权文件；发现边界外缺陷提交文件/行号/反例/影响，暂停受影响工作并交用户决定。

### 11.2 交付文件

`docs/block4_acceptance.md`中文写“完成内容 / 验证方法 / 踩坑记录 / 已知限制”；附每测试输入/预期/执行结果/证据、G-ABI与G-B批准原文/时间/版本、D16/D18处理、其余D遗留、raw非回归差异和actual耗时/剩余项。不因main合入Block3将其WIP记录改完成。

该文件必须新增独立的“决策记录”节，完整记录以下内容，不能只放链接：

| 决策 | 必须记录的正文 | 状态维护 |
|---|---|---|
| G-C | 2026-10-06用户确认本期不做置信度、v0.1保持二值输出；完整写入§3的四条原因及v0.2七步启用条件 | 已确认；本期不实现，后续独立需求 |
| G-ABI | 用户选A，DisplayState在原字段之后追加std::string value；这是ABI例外，FrameResult嵌套布局变化；相关目标全部重建 | 已批准，可直接实施；记实际落地与验证位置 |
| G-B | 两个预算当前待批准，Codex按§6.3测量/提交表，用户选择后写生产；未批生产NOT_READY、不填经验值 | 批准前写待批准及产物进展；批准后保留历史并追加原文/值/版本/范围 |

`src/tushenghao/docs/block4_budget.md`必须包含§6.3的定位统计、平滑收益/偏离/回退对照、可重放命令、原始证据路径/hash、支持范围和审批状态。只写两个数字或“经验值”不算交付。

README包含编译、temporal_audit每参数、输入与输出位置、配置单位/缺预算NOT_READY、raw/track/display隔离、循环reset、模块导航、实际支持范围及排错原因码。注释在每个改动类/函数或关键逻辑处说明“原来什么问题/为什么这样改”，不逐行翻译、不只写“优化/修复”。

必需交付为两份Block4文档、README、`src/tushenghao/docs/INDEX.md`与`src/tushenghao/tools/INDEX.md`更新，缺一不可。即使测试全绿，文档缺失仍为未完成。

允许Codex增补必要文档，例如时序调参指南、故障排查。放在`src/tushenghao/docs/`或其已有适当分类，必须登记`docs/INDEX.md`并使用可用相对链接；不得散放仓库其他位置。该权限不允许修改`docs/ref/`冻结原文或覆盖旧历史证据。

实际耗时记录：`步骤｜开始/结束｜主动耗时｜验证命令/退出码｜剩余项｜阻塞/批准依据`。冻结估计6小时40分～10小时30分只是评估；本方案多了核查/共享接口适配，不保证装入原盒子，不能自行删范围。硬节点沿用10-06干净Linux复现、10-07提交，由用户排期。

### 11.3 可直接交给Codex的执行提示词

```text
任务：仅实现Block4输出稳定层，依据本方案以及src/tushenghao/docs/ref/4.md。
基线main 08c5c44，用户已开feat/temporal-stability；你不执行任何Git，
包括构建期间的git rev-parse。按方案替换CMake为显式MARK_COMMIT_LABEL。

先完整读本方案与4.md、接口汇总、D01-D25原审计；遵循§4文件白名单。
六个lib模块不变，temporal组件放lib/pipeline；不建lib/temporal或万能utils。
decode_stage保持无状态且不改；process/audit通过同一finalizeDecodedFrame接入。

G-ABI已批准选A：DisplayState追加std::string value，记录FrameResult嵌套ABI变化，
直接实施、全部目标重建，不再问许可。G-C已确认：Block4不做confidence，
v0.1保持二值输出，v0.2另立七步置信度标定需求。
仅G-B实际数值待批准：按§6.3跑实验，提交平滑收益/偏离/回退次数表，
用户选定后填写生产。未批生产一律NOT_READY，关闭平滑也不能绕过；
实验fixture/候选配置独立归档，不填经验值。
已有冻结14ms/.7/50ms/.5对角线/面积.5~2/5帧桥接不再重新讨论。
G-B等待只阻塞生产依赖工作，继续无依赖骨架/显示桥接/主动测试。

每个实质改动写清中文“旧问题是什么、为什么这样改”。四个公共调用签名、
Detection/FrameResult直接字段不变。DisplayState仅按批准例外增加value。
不改geometry/corner观测/语义/有效性阈值，严禁用历史修复118帧竞争拒绝。
confidence与marker_code生产为null，不添加评分/概率或有效ID。

按§5-7实施三状态、对应先于平滑、原始参考关联、失检立即无track、
恢复首帧原始、unknown不回填、非法清所有状态、bbox从稳定点重算。
内部稳定点按当前原始slot缓存；对外排序另做。未知方向用纯几何环排序接口，
不伪造P0-P3；稳定几何检查不造CornerEvidence调用原validator。
生产显示语义源恒空，A/B桥接仅合成验收。

按§8步骤执行并逐项完成§9主动断言；Release也必须失败可见。
不删旧测试、不改期望换绿；过期NOT_READY仅对完整就绪fixture升级，
缺预算/非法输入负例保留。raw逐帧不变、旧H通过ID保留，empty帧无track。
audit每帧仅decode一次，输出完整对照/原因/hash；未跑写NOT_RUN。

交付中文docs/block4_acceptance.md、block4_budget.md、README与工具索引，
包含批准、测试与回归证据、踩坑、已知限制、实际耗时/剩余项。
所有强制项通过且G-B已批才标Block4完成；必需文档缺一不可。
验收文档必须有“决策记录”节：G-C四条原因和v0.2七步、G-ABI选A/ABI例外、
G-B实验/待批准或已批准状态完整记录。补充文档可放docs现有分类或新建，
必须登记docs/INDEX.md，不得散放。没有V/Q不宣称视频准确率。
不实施Block5诊断/计时框架，不引卡尔曼/预测/多目标/新依赖。
发现白名单外问题或冻结冲突：给文件/行号、最小反例及建议，等待用户裁决，
不得自动改冻结文档、扩范围或降低标准。

每轮四行汇报：已完成 / 实际验证 / 未完成与阻塞 / 下一步。
最后列实际修改文件、验证命令/退出码、证据路径和未闭合问题，不执行Git。
```
