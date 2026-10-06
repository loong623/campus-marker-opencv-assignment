# 三L观察层修复与正式回归

2026-10-06，执行用户批准的parent-v2-diagnosis.md四条方案。最终冻结[配置v7](../frozen_detector_v7.yaml)，所有数值与上一轮v4相同；仅观察/搜索规则改变。**H720/720 PASS、C720/720；全视频864/1676帧产生阶段Detection，旧591保留581。** [适用边界与已知局限](适用边界与已知局限.md)为本轮交付的一部分。没有commit。

## 固定规则及修改范围

1. `geometry_l_topology.cpp`从工作图完整实际轮廓构造六边L候选。固定epsilon为原2工作px乘{1,0.5,0.75,1.25,1.5,2}；简化6–8顶点时只允许省略至多2个实际顶点，完整原弧到实际简化弦仍需在当前epsilon内。实际6顶点必须五凸一凹，轮廓绕向归一；两条内臂分别长于平行笔画宽度（同边族投影比0<a,b<1），区分短M而不靠顶点数量。无模型预测顶点，也没有全网格调阈值。
2. `geometry_observation.cpp`以显式候选判断是否支持L，保存实际来源ID及多拓扑；`geometry_matcher.cpp`枚举各候选、原简化轮廓所有实测凹点并按实际坐标去重，逐三L组合/模型对应/锚点元组求原三点仿射，拒绝非有限、奇异及镜像。
3. 原1000搜索预算现在覆盖每次实际锚点元组尝试，包括无效仿射；精确K完成不截断，确有下一分支才截断。`geometry_validation.cpp`仅修来源ID查找和诊断传递，5工作px的各L投影顶点平均到观测轮廓距离、面积0.5–2、边界不完整规则均未放宽。
4. 白阈值200、resize、M/S/角点全部数值、模型坐标、三L模型与仿射设计、公共接口/稳定层不变。旧合成器、参考终稿/冻结3-2/接口总汇总未编辑。

观察层不额外以epsilon强制外角闭合：第一轮v5该检查导致121个旧检测帧失去父，完整失败结果保留。v6移除额外闭合检查后H720、视频976，但1573仅枚举候选凹点，漏掉原简化轮廓已有实测分支，P1原图指定外边失败。最终v7补全原轮廓凹点并按原validator筛选；1573恢复有效四角，但存在另一个未排除失败分支，按原竞争规则保守empty；不保证新增分支必然增加检出数。数值始终不改。所有历史冻结/逐帧结果保留在[原始归档清单](L-observation-raw-archives.json)，不覆盖此前v4。

## 逐项验证与全H防回退

新增主动C++检查包括真实圆角L3恢复、真实短M1不冒L、帧2实际轮廓不被观察闭合误杀、非首凹点枚举、拓扑子序列不遗漏原凹点、精确48/47资源边界、非连续来源ID、绕向/矩形与灰度200/201。修复前3项反例见[红记录](07-l-topology-red.txt)，最终9项见[绿记录](07-l-topology-green.txt)。旧manual_validation_check夹具三个默认ID均0，已补唯一ID，不放宽生产重复ID拒绝。

两个Release完整构建与CTest均16/16，配置check、diff检查通过。CTest程序数不等于覆盖全部场景；旧测试部分assert在Release失效的限制继续披露，新增检查主动失败。[H逐ID防回退](L-observation-H-nonregression.json)保留原689与前705全部通过ID。最终H无噪声144/144、噪声576/576，各6个噪声分组96/96；隔离720/720，错角/方向/屏幕排序/bbox/证据差异0，最大真值误差1.3462912原图px。正式runner退出0，独立报告H_acceptance=true；C stage/isolated720/720且wrong_valid0（C是诊断回归，不冒称独立H）。

## 全视频归因与资源

| 类型 | 帧数 | 数据含义 |
|---|---:|---|
| 已有阶段Detection | 864 | 当前帧四角/证据/语义链路通过；未标注不宣称正确检测 |
| 亮度适用范围外：无白组件 | 325 | 固定白阈值200，原图和工作图白像素均0 |
| 观察层不足三个L | 239 | 包括目标不完整、分割/拓扑不可用等；无标签不能细分为真值漏检 |
| 父验证通过但未完整补全 | 24 | 原父不完整或M/S全边界/拓扑/面积/方向约束未通过 |
| 原图角点取证拒绝 | 106 | 指定外边实际支持不能同时满足连接、位置、方向、长度及拟合预算 |
| 竞争候选尚未排除 | 118 | 至少一组有效四角，但另一个父/补全分支取证失败；按既有规则保守拒绝 |
| 后续语义/证据拒绝 | 0 | 有效测量存在但未形成可输出结果 |

[完整1676行](video-v7-all-frames.csv)、[失败原始诊断](video-v7-failed-frames.jsonl)、[H/video报告](H-video-v7.md)及全部压缩原始支持弧、拟合、父候选/逐顶点残差归档。三L验证父1112帧；相对旧771帧全部保留，旧323残差拒绝中321恢复父、旧257少L中20恢复父，另2帧现在显式L不足。父验证改善不等于同数量检测改善，后续补全/角取证仍可拒绝。

检测帧864、新增283、旧591保留581、旧检测回退[2, 13, 190, 194, 411, 493, 686, 755, 1052, 1573]。方向known864帧、unknown0帧；资源截断0帧，单帧锚点尝试最大48，低于1000。325暗帧范围/灰度与此前一致，不改阈值；未标注不判漏检。pipeline mean97.652/P99 428.371/max985.520ms，不含解码，没有硬实时结论。M/S压力8场景历史[resources-v5](../04-four-track-v3-v4/resources-v5.md)及当前精确锚点预算测试分开作为证据。

## 复现与归档

```sh
cmake -S src/tushenghao -B build/block3-L-clean -DCMAKE_BUILD_TYPE=Release
cmake --build build/block3-L-clean -j 4
ctest --test-dir build/block3-L-clean --output-on-failure
build/block3-fixture/block3_verify build/block3-H-v1 src/tushenghao/docs/evidence/block3/frozen_detector_v7.yaml build/block3-H-replay-new.jsonl
build/block3-L-clean/decode_audit data/raw/marker_video.avi all src/tushenghao/docs/evidence/block3/frozen_detector_v7.yaml > build/block3-video-replay-new.jsonl
```

C/H样例沿用原manifest和240张图，未重新渲染。冻结数值与生产相同，仅模型相对路径/注释不同；[源文件/模型/配置/视频hash](L-observation-provenance.json)、[原始与gzip双hash](L-observation-raw-archives.json)和全部构建/runner/测试日志随evidence保存。模型SHA保持844853098082c0eae1972f744e084ca8b928233bf62b6c3933c6af4ada374109；HEAD25b6cbe不变，保留用户.gitignore，不git add/commit。

本轮批准的观察层修复与规定回归全部完成，H门槛闭合。V按用户跳过；未标注视频正确率/Q、原32负例及总体§9.1、模型绑定配置化仍未闭合，公共process仍NOT_READY，整体Block3保持WIP。

逐帧全部实测L拓扑、epsilon、实际凹点锚点及所有父逐顶点残差另存[完整观察/锚点原始记录](parent-v7-full-topologies-anchors.jsonl.gz)，1676条；与正式父阶段摘要完全一致。
