# M/S 拓扑与旧三L验证：独立修复、反例与剩余阻断

本项依据用户“完整修复、最小范围”批准执行。角点预算与原图弧搜索由其他独立项处理；本项未改变任何生产数值门限，未改 `geometry_validation.cpp`、三L匹配/结构观测、模型坐标或公共接口。独立重放使用 `frozen_detector_v2.yaml`，因此本报告数字仅表示前半链路的验证/补全，不表示最终 Detection 或人工标注正确率。

## 固定拓扑反例及改动

`H-r0-a60-s0/w480` 的 M1 工作轮廓只有23个CHAIN_APPROX_SIMPLE实测点，面积53，三L中位面积144。原五个简化比例的 epsilon 为1、1.5、2、2.5、3工作px，均得到4顶点，已经删除细M凹口。更细的0.75px得到7顶点，但严格要求6点仍会拒绝。此前“7→5”的推测不作为最终归因；实际顶点数见固定轮廓及重放。

仅修改 `assignment_match_metrics.hpp`：

- 保留原五比例，额外加入固定1/4上限比例（冻结上限3工作px对应0.75px）。
- 简化结果比目标多1～2顶点时，枚举保留目标顶点的子序列；输入最多8点，最多枚举256个位掩码。每个顶点仍是原轮廓实测点。
- 对候选每条合并边覆盖的完整原始轮廓弧，验证所有原轮廓点到有限段的距离不超过既有 epsilon 上限3px。凹凸绕向、循环/反向对应、方向门限30°、完整原轮廓双向边界9.5px、面积误差0.045、采样步长1px仍全部保持。未用模型投影生成新顶点。
- 原本可用的候选全部保留；新候选只扩充构造，不删除旧分支。最终689条原H成功集合是否保持由统一正式重跑核对。

固定反例文件：`tests/data/ms-topology-skipped-six.txt`（H）与 `tests/data/ms-video638-topology.txt`（实际视频638）。实例mask只在工具侧确认H组件2确为M1，未送入Detector。旧冻结库运行相同测试，两项拓扑反例均失败、其余6项通过，退出1；当前库8项全部通过，退出0。证据为 `05-ms-topology-red.txt` / `05-ms-topology-green.txt`。

## 独立31条H失败重放

基线31条均没有完整补全。改动后：16条 M 拓扑反例全部出现1个完整分支；15条旧三L验证失败仍为生成6父、验证0父。原图角点和最终正确率不在本阶段统计中。

15条包括圆角r2、角度45/135/315、work1440、各5个噪声种子。最佳父中两个L的单顶点距离通常0～3.2px，而另一个绑定的“L”有四至五个顶点距原边界32～34px，平均残差21.94～22.38px，确实超过既有5px上限。实例mask确认真L（例如45°的L3组件4）在观测简化后被分类为M，真M1组件2反而被分类为L并用于拟合。旧validator正确拒绝错误绑定，放宽残差会掩盖上游结构错误。依用户“若模型结构问题暂停”要求，三L类别/锚点设计改动暂停，详见 `parent-v2-diagnosis.md`、`parent-v2-H15-anchor-diagnosis.jsonl`、`parent-v2-H31-instance-identities.jsonl`。

## 独立58帧视频重放

选集严格来自冻结v2全视频的“有validated父、assignment输出0”58个frame_id，未添加或删去分母。先运行正式前半链路，后测量所有未占用组件针对指定M/S的原始指标，不使用待审门限过滤统计。

| 原始阻断类别（互斥） | 帧数 | 本项结果 |
|---|---:|---|
| 正行列式父已标CLEARLY_INCOMPLETE | 18 | 保持拒绝；全部有绑定组件接触工作图边缘，1577还超面积比 |
| 有完整父，但M/S完整有限边界距离超过9.5工作px | 39 | 保持拒绝，不放宽边界预算 |
| M拓扑候选构造缺失、边界/面积门限内 | 1 | 帧638恢复1个完整分支 |
| 合计 | 58 | 1恢复，57仍有独立真实拒绝条件 |

39帧进一步互斥细分：M单独边界超限11帧，M+S1b超限16帧，M+S1a超限7帧，M+两S均超限4帧，S1b单独超限1帧。原始数据保存了所有父、所有候选，故可逐帧复核，不能把39帧称为拓扑bug。

具体数据：帧142 M边界11.68px > 9.5，面积误差0.0249且拓扑可用；帧638 M边界2.66px、面积误差0.0044，原五比例拓扑缺失，修后补全成功。帧266/273/274虽原始M/S几何指标能匹配，但L绑定接触工作图边缘，父已CLEARLY_INCOMPLETE；绕过该标志会违反裁切保守拒绝。帧36同样为父边界裁切，并非M/S入口的代码bug。

因此本项不能宣称“58全部修好”。还需分别处理裁切适用条件与真实父仿射投影偏差/实拍边界预算，不能靠拓扑枚举代替这两类约束。视频无V标注，不据潜在分支数量推断召回或定位正确率。

## 归档与复现

`ms-H31-before.jsonl` / `ms-H31-after.jsonl` 保存原始轮廓、生成父矩阵、每顶点投影残差及每M/S原始指标；`ms-video58-before.jsonl` / `ms-video58-after.jsonl` 保存58帧同类数据，before另含冻结v2正式阶段诊断和阻断类别。SHA256及所用库摘要见 `ms-stage-manifest.json`。before库冻结到 `/tmp/block3-v2-detector.a`，after库为独立 `build/block3-ms/libmark_detector.a`。

诊断工具源码归档为 `ms_stage_diagnose.cpp`（仅工具，不链接Detector）。可用当前库编译：

```bash
c++ -O2 -std=c++17 src/tushenghao/docs/evidence/block3/ms_stage_diagnose.cpp -Isrc/tushenghao build/block3-ms/libmark_detector.a $(pkg-config --cflags --libs opencv4) -o build/ms_stage_diagnose
build/ms_stage_diagnose src/tushenghao/docs/evidence/block3/frozen_detector_v2.yaml src/tushenghao/docs/evidence/block3/H-v2-all-failure-diagnostics.jsonl build/ms-H31-replay.jsonl
```

工具只对选集前半链路做重放，后置原始测量不反向改父。所有输入SHA与正式C/H/视频统一验收见root最终报告；本报告不代替统一回归。
