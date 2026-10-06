# Final-fixes 验收记录

状态：代码与自动验证完成，待人工 review；不代替用户最终验收、commit、push 或 merge。
依据用户批准终稿，R01–R09 的位置/根因/修法直接使用终稿，不另找原报告。
执行前只读 HEAD/ref 确认 feat/final-fixes、c7fffe4b07438fcb44d032f51f3af221e4f174f1。
未执行任何 Git 命令。G-CLEAN 按用户明确决定记未验证，并审查依赖和流程完整性。

## 1. 完成内容

按 A→B→C→D 实施，每批验证后继续；C 保留单独拆分前后证据。

| 项目 | 实际落点与验证 | 状态 |
|---|---|---|
| F01 / R01 | input_completion、runner、CLI/config、check-run；截断 262144 字节实际读 41/期待 1676，保留记录/INCOMPLETE，App 与核验均非零 | 自动 PASS |
| F02 / R02 | compare 拒绝空/空、空/非空、短数据配 1676、重复/倒序；合法子集仅声明 compared_records_only；期待数要求完整连续 ID | 自动 PASS |
| F03 / R03 | 新增 portable debug/verification YAML，模型相对配置定位；复制源码到全新 /tmp 路径重新构建并运行三模式，实际模型 hash 相同 | 自动 PASS |
| F04 / R04 | README 列出 build-essential/cmake/libopencv-dev/libssl-dev，区分独立 JSON 工具及可选 ffprobe；本机依赖/CMake检查无遗漏，新路径通过；无干净 rootfs | G-CLEAN 未验证（用户决定） |
| F05 / R05 | complete_export 强校验并补首帧/分母/侧表；开关×选中/未选中视频矩阵，计时关为 DISABLED/null，无导出 NOT_EXECUTED | 自动 PASS |
| F06 / R06 | geometry scope 在角预算门控前结束；缺角预算 Geometry 独立，Decode/Full/Public 仍 NOT_READY | 自动 PASS |
| F07 / R07 | Block4 43 条引用改为现存历史归档；links 工具全部通过，历史文件 SHA256 不变；编码/空格/尖括号/锚点边界检查通过 | 自动 PASS |
| F08 / R08 | README、INDEX、block3、诊断 schema 及 public diagnostics 注释纠正；当前/历史测试状态分开，保留真实 WIP 和未知项 | 文档核查 PASS |
| F09 / R09 | runner 会话拆分、serializer 分段、verifier 会话/compare helpers，renderer 格式整理；C 前后短序列/计时关/子集全记录去身份和实测耗时后 exact | 自动 PASS |
| F10 / fixture | 真实模型三 L 场景经 observeShapes，稀疏组件 ID/正确对应、退化负例及资源截断；Release 主动 check，原 matcher 目标保留 | 两种构建 PASS |
| F11 / display | 独立 sibling probe、无屏/坏 DISPLAY 降级，主进程继续；运行时 cv::Exception 回收窗口；q/ESC 标 USER_STOP 不完整 | 自动 PASS；G-DISPLAY 待人工 |
| F12 / video | 固定 FFmpeg MJPG→MP4，所有当前帧写入，release 后完整读回才改最终名；完整 1676/1440×1080/70.408 FPS、codec=mjpeg；损坏仅 partial | 自动 PASS；G-VIDEO 待人工 |
| F13 / 中文 | 保留英文技术行，中文完整性/数量/平均/p95/总耗时/测得算法瓶颈；计时关不造 0，损坏/零帧/打开失败和 CLI 边界核验 | 自动 PASS |

实际源码文件：App main/offline_runner/debug_renderer 和新增 input_completion、display_session/display_probe、
video_exporter、run_console；内部 config/observability_config、decode_stage、diagnostics_recorder；
原 matcher/diagnostics/integration 测试；observability_verify 和新增 final_fixes_verify；CMake 新 helper/工具连接。
新增工具没有增加第 24 个 CTest。配置只增加 App 字段及便携文件；中文注释说明原问题及改法原因。
public detector_types 仅纠正 diagnostics 注释，Detection/FrameResult/DisplayState 布局及四个公开调用签名未改。
六个 lib 模块保留；geometry/corners 算法、生产阈值、模型坐标、时序预算未改。
具体改动文件及最终源码 hash 见[保护检查](evidence/final-fixes/environment/protected-check.json)与
[源码清单](evidence/final-fixes/environment/final-source-manifest.json)。ref 与历史 Block3/4/5 逐文件对修改前 hash 核验。

## 2. 验证方法

实际环境 Ubuntu 24.04.4、GCC 13.3、CMake 3.28.3、OpenCV 4.6、OpenSSL 3.0.13。
依赖查验、构建、测试、App/验证工具的真实 argv、时间和退出码见
[完整命令索引](evidence/final-fixes/archive/commands.json)，stdout/stderr 均保留。
必需依赖在 CMake find_package 中显式声明，README 安装列表覆盖；ffprobe 与独立 fixture JSON 依赖单列。
新路径复现只使用当前主机依赖，不冒称已执行干净 Ubuntu 安装。

输入原视频 SHA256 aa1219a7a7b702ea1265be8752853a267c982a651afa0f7846f516f9517c9ac7，1440×1080、1676 帧。
golden 原 SHA256 0f2a0cb8c80146124a18d6e7ff72a290d225a1266f7b5a36fb3249c3cc49e235；
修改前确认 1676 行、连续 ID 0…1675，并以工具自比确认 c1785fb03a54ed25。
实际模型 SHA256 844853098082c0eae1972f744e084ca8b928233bf62b6c3933c6af4ada374109。

最终 Release/Debug 均 23/23；[Release](evidence/final-fixes/regression/ctest-release-label-final.stdout)、
[Debug](evidence/final-fixes/regression/ctest-debug-label-final.stdout) 为末次构建结果。
A、B、C 各批两种构建结果也单独保留。Debug 配置三模式短序列启动均退出 0；
损坏样本读 41 帧、检测 22/空 19，不完整退出 1，check-run 拒绝。
新用户路径独立构建 23/23，三配置检查及 baseline/debug/verification 10 帧均完整，模型实际解析至新路径。

唯一 baseline 全视频测量：1676 帧、864 DETECTED/812 NOT_DETECTED、FNV c1785fb03a54ed25。
公共 process 平均 96.238 ms、p95 279.303 ms，运行总耗时 162.918 s；测得均值最大阶段为 decode 94.931 ms。
不把三模式差额称精确诊断成本，不把源 FPS 称算法速度；没有优化或声称达到 14 ms。
Release verification 同时完成全帧逐帧结构/原语义原因比较、Export 侧表、完整视频和中文摘要验收，
比较不用 route-b，结果 1676 帧零失败、指纹相同；完整 run 的 check-run 提交/选中均 1676。
Debug 全视频也完成 1676 帧、864/812、c1785fb03a54ed25；归档完整性和逐帧结构比较 PASS，零失败帧。

完整视频采用无窗口正常收尾路径；之后两项 App 收尾修改只收紧 GUI 计时归账与未知异常文案，
不改变该路径公共结果，末次双构建 23/23 包含相应反例；manifest 保留每次实际源码摘要。
视频复用同一次 verification，未为导出重复全视频运行。
归档 MP4 用 OpenCV 逐帧读回 1676、1440×1080、70.408 FPS，ffprobe codec_name=mjpeg、tag=mp4v。
实际 run、全量 JSONL、侧表、失败标记、短/损坏输入、报告、日志及完整 MP4 均保留于
[本轮证据](evidence/final-fixes/INDEX.md)。按归档路径重新核验全量结果、输入完整性、视频和链接。
manifest 保留当时 source/model/build 路径，不改写成假的可移植事实；短 run 复制不是自包含重放包。
SHA256 清单排除自身避免自引用；归档核验后只清有精确标记的本轮 build 与临时源/工具，其他旧 build 保留。

## 3. 踩坑记录

容器元数据可能仍写 1676，真实读取却只有 41。EOF 只是读取停止，不能作为完整性结论；
显式期待数覆盖判定而保留原 metadata，未知期待数不能默认为 PASS。子集缺失与全量损坏分别记录。
原空比较会零次循环后 PASS；现在空数据主动拒绝，期待数及连续 ID 将短/重复/倒序假回归挡住。

导出计时自引用无法在写出自身前完成，因此 frames.jsonl 先写 NOT_EXECUTED，完成值以同帧侧表关联；
首帧补记、状态分母和一次性约束同时核验。关闭计时仍有真实导出，状态为 DISABLED/null，
侧表自身与 writer 打开/收尾归运行级成本，不递归伪造帧耗时。
显示降级后保留的末次 GUI 耗时不能再次归到后续离屏帧，收尾已加本帧实际进入显示判断并重跑两种 CTest。

MJPG 放入 MP4 时 FFmpeg 转换容器 tag 为 mp4v，其日志不是编码切换证明；
实际 codec_name 仍是 mjpeg。writer.write 返回 void 不证明落盘；必须 release 后完整读回再发布。
坏 DISPLAY 可能令 Qt abort，普通 cv::Exception 不能拦截，因此 GUI 启动在独立进程先探测，
超时仅回收自己启动的 PID，缺失/失败都降级。生产算法仍在主进程正常逐帧执行。
未知异常不按 video 子串猜测类别；收尾改为明确异常映射，并增加未知且含 video 的主动反例。
模型相对 YAML 所在目录解析，搬迁不能复制旧构建缓存或假借原个人路径；三份 portable 配置已实跑。

## 4. 已知限制

812 空帧是真空还是漏检尚无标注，待用户通过 display 目视；864 不是准确率。
约 96 ms 的现有性能未优化，比赛 14 ms 目标未验收；一般透视、距离、置信度等既有范围限制保留。
仅覆盖指定 262144 字节截断、零帧和打开失败等边界，不承诺所有损坏格式。
GUI 服务在探测与主窗口间或运行中强制消失不能作绝对存活保证，普通 OpenCV 异常有降级处理。

G-DISPLAY 待人工：按 README 的 debug --display 命令确认窗口、连续画面与当前结果图层；
q/ESC 应非零退出、INCOMPLETE 且有视频时只留 partial。
G-VIDEO 待人工：用用户播放器打开[完整 MP4](evidence/final-fixes/regression/verification-release/overlay.mp4)，确认可播放。
G-CLEAN 未验证：无可用干净 Linux 环境入口；用户明确要求保留未验证并审查理论依赖/流程完整性，已执行。
这些状态不填写最终 PASS；本轮交付供用户 review，Git 操作由用户完成。

输入路径不存在或 CLI/配置在创建 run 前被拒绝时，只保留退出码与 stderr，不保证有 run 目录；
文件存在但解码器无法打开、以及已打开却零帧时，FAILED 生命周期已实跑验证，不生成成功 summary。

## Gate1／Path A 定点修复（2026-10-06）

三L锚点逐候选资格修复，decode保守竞争规则保持。原基线Release/Debug23/23；本次经用户批准补manual mock来源后，两种构建均25/25，A01–A13全部通过。初次24/25的受阻日志保留。
Release完整1676及专项机器核查PASS，864→976，旧A118恢复112、残留6，旧成功零退化，A外状态零变化；20个旧成功四点改变、方向0变化、132帧review证据已归档。Debug完整1676及专项核查PASS，两构建逐帧公共结果零差异、指纹均0d2d2e63aef753bc；用户已确认132帧人工复核全过，Path A本轮验收通过。
原13项报告保持，原工程通过不代表整个闪烁解决。PathB／564无假设延期fix2。详见[专项验收](path_a_acceptance.md)、[施工日志](path_a_fix_log.md)及[证据目录](evidence/final-fixes/path-a/)。
