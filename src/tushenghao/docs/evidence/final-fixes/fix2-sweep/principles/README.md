# fix2-sweep 原理研究证据

本目录为只读研究产物，不加入项目 CMake，也不提供可启用的生产配置。原报告在 previous_report.md；新增部分在 research_append.md，并已追加至 docs/fix2_sweep_report.md。0.75/9.5 与两遍重试是前一轮历史敏感性结果，本轮未实施、未作为推荐。

## 固定调查与证据

- 历史预算：synthetic_C_v1/v5_measurements.jsonl 保存本地 build 中的历史原始记录；historical_C-v1/v5-summary.csv 为对应历史分层表副本；budget_recomputed_v1/v5.csv 和 budget_recompute_check.json 为重算结果。
- 实拍测量：real_raw_residuals.jsonl，112 个首个失败角点全部有记录，111 可测、784 保留位置失败。去重实验也是边对层级，不能当整帧 Detection。
- 成像对照：controls/original 的 16 张 PNG；control-qp0/28.mkv 与 decoded-qp0/28 的解码图；controls-*.jsonl 每种 64 条角点结果。16 条件按两个中心、两种圆角、四种近轴角度预先固定，无噪声，模型 scale=2。qp0/28、yuv420p、preset medium、g=1 固定，不根据失败追加更大 QP。
- 对照真值边的严格关联检查有 16/19 条未通过，保留全部分母，未将所有结果认作合格 C2 样本，不由该实验推新预算。
- 法向九点剖面：profile_targets.txt 保存原图与每条支持弧输入；photometric_profiles.jsonl 和 photometric_summary.json 为输出。中间灰度定义 (20,235)，颜色差定义 max(BGR)-min(BGR)>20；这些是描述统计定义，不是检测参数。
- distribution_summary.json：分布、重复像素、噪声支持弧相同性及对照统计；parent_angles.json：父仿射的 x 轴投影角，不等同相机真值姿态。
- tools/copy_provenance.json：分析副本与正式拟合代码之间的差异；tools/*.log 和 *.stderr 保存编译/执行日志。最初两个编译尝试缺少标准头文件，failed.log 留作执行记录；补齐仅在本目录工具中，最终编译运行成功。

## 分析副本的边界

raw_measurement.cpp 只在隔离测量函数里移除被测均值残差的上限判定，其余原冻结边对约束、身份引导与歧义拒绝仍在；不发布 Detection，不修改配置文件，不进行生产重试。

unit_pixel_weight.cpp 是取样权重的反事实：每个不同像素等权拟合/计算均值，保留全部原始轮廓作证据。它没有被接入生产 validator；与当前按访问序列复算的 validator 不同，不能直接作为可部署修法。生产若将来改变定义必须同步契约、工具、拟合及 validator。

## 复核

从仓库根目录运行（无需重新构建或跑视频）：

```bash
python3 src/tushenghao/docs/evidence/final-fixes/fix2-sweep/principles/tools/verify_principles.py
```

脚本分别重算两版 C 的 36 层，对照各自历史表并检查本轮关键计数、1315 个保护输入、743 个原分析产物以及报告历史前缀。旧的 analysis_hashes.json 的 report_sha256 对应 previous_report.md；当前整份报告 hash 在 final_check.json 和 hashes.json。

分析工具的编译依赖为 C++17、OpenCV 4.6、系统 nlohmann/json、libcrypto，以及已存在的 build/block3-fixture/synthetic_ref/libsynthetic_gen_core.a（只读链接）。raw_real.cpp 需链接正式 corner_observation.cpp、corner_edge_fit.cpp 和两个分析副本；imaging_controls.cpp 链接正式 corner_edge_fit.cpp、raw_measurement.cpp 和原 synthetic core；photometric_profiles.cpp 只需 OpenCV。包含路径为 src/tushenghao/lib、src/tushenghao/include、src/tushenghao/tools/synth_ref/include。

调用参数及输入为：

```text
raw_real <fix2-sweep目录> <path-a/verification-release/effective_config.yaml> <config/marker_geometry.yaml> <fix2-sweep/tools/replay_targets.txt>
imaging_controls generate <新的original图像目录> original
imaging_controls measure <原图或解码图目录> <mode名称>
photometric_profiles < profile_targets.txt
```

工具从 stdout 输出测量 jsonl，生成器拒绝覆盖已有原图；如重跑，所有输出只能用 fix2-sweep 下的新目录，不能写 path-a 或覆盖旧记录。控制序列编码命令的固定参数为 `ffmpeg -framerate 10 -i input-%02d.png -c:v libx264 -preset medium -qp 0/28 -g 1 -pix_fmt yuv420p`，解码使用 `-start_number 0`。没有安装依赖，也没有更改项目构建。此次三个可执行文件已清理，仅留源码、日志、图像和记录。

## 研究结论与停止条件

0.5 计算正确；平均垂距公式与现行定义一致。模型缺少实拍灰度过渡、颜色与空间边界变化，原 σ1 不足以模拟阈值后的真实边界。去重和简单编码/近轴控制都未充分复现或解决问题；不唯一归因某种硬件。标定去重点数和几何占位 residual 是已确认的独立工具/诊断问题。物理角定位误差不能由线残差替代。

本轮不再扩大参数搜索。后续先固定成像支持域与同口径 C2/H2，再验证物理角精度、负例安全及独立真实数据泛化；实施及数值仍需用户审核，当前没有生产修改或正式验收结论。
