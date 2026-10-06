# 开发与验收记录

验收日期：2026-10-04（Asia/Shanghai）。工具开发、自测和可读性检查已完成；正式数据交付 **BLOCKED**：`inputs/approval.json` 不存在，模型与实验没有有效批准证据。没有宣称 Block 2 或冻结 Step 4 结项；检测算法验证、实际误差、拒绝结果、支持范围、内部资源计数和容差确认仍需外部算法接入。

开发起点 2026-10-04 00:02:20 +08:00，验收计时截至 00:41:50 +08:00，实际墙钟39分30秒，包含等待依赖到位和构建时间；不是 Detector 性能测量。用户补齐 nlohmann_json 与 OpenSSL 开发依赖，工具没有自动安装软件。

## §7 顺序记录

| 步骤 | 产物与状态 |
|---|---|
| 1 输入/目标/依赖 | 四份原文 hash、sources 别名与状态、独立 CMake 核心库/CLI/CTest；依赖最初缺失已明确报告，用户安装后构建成功 |
| 2 模型转录 | model 私有JSON，Markdown表格逐点/锚点核对，模板边长/落位/外角与编号检查；面积由顶点算，M维持条件模型 |
| 3 变换/网格 | 已知仿射、逆转置笔画宽、完整枚举；plan 38,784项，无正式栅格实验 |
| 4 几何/渲染/真值 | 支撑线偏移、顺逆绕向、非法记录、8×8并集、裁切、16位实例掩膜；独立手算测试通过 |
| 5 异常 fixture | §5全部64项构造与纯几何检查；图像生成时的桥/白片连通检查已实现并用独立图形验证；正式MARK产物待批准 |
| 6 CLI/记录/文档 | validate/plan/generate/verify、INCOMPLETE事务、规范化hash、重建验证、README与协议 |
| 7 selftest/验收 | Release CTest 7/7；两个新run重复自测和verify通过；A1–A13证据如下 |
| 8 formal/verify | **BLOCKED**，无批准命令返回3且未创建正式run；未生成任何MARK正式图像，不自行签署批准 |

## A1–A13 逐项自检

“PASS（工具）”只表示本工具的自测/实现检查通过；每项中需要有效模型批准或真实算法的部分，明确留待后续，不以工具通过替代它们。

| 编号 | 结论 | 已执行检查与证据 | 保留边界 |
|---|---|---|---|
| A1 | PASS（工具） | inputs/sources.json、docs/source_mapping.md；四份原件SHA-256未改变；configs带来源；sources限制仅四份交付原件 | 无视频参数来源，无原文改写 |
| A2 | PASS（工具） | CTest workflow：缺记录/错误hash均抛退出码3、对应目录不存在；build-acceptance/test-evidence-workflow.json；实际 CLI formal 缺批准退出3 | 临时副本只写明确无效TEST_INVALID_NOT_APPROVAL记录，工具inputs中未写批准 |
| A3 | PASS（工具） | CTest model：三模板六片与原文顶点/锚点/边号/外角对应，缺项、重复、自交、边长错误拒绝；L/M/S面积由顶点和推导式比较 | M条件面积不是Detector门限；正式模型资格未获批 |
| A4 | PASS（工具） | transforms：整圈、各比/宽度、相位、顺时针、面积缩放、剪切奇异值比、奇异/零分母拒绝；grid：全部ID唯一；runs/acceptance-final-plan/manifest.json 数量 | 6912基本、34560全局、4096局部、64剪切；未宣称支持范围 |
| A5 | PASS（工具） | raster：独立整数矩形半覆盖128、四分之一覆盖64、半像素矩形四个完整像素、L凹部、实例65535；mask严格gray>127 | 没把连续面积等同像素面积 |
| A6 | PASS（工具） | perturb：顺逆绕向膨胀/收缩、未移支撑线不漂移、坍缩/穿过骨架无效；已知局部偏移保持L0.v3身份并保存新交点 | 非法样例保留，visible为空；不用名义锚点替代扰动交点 |
| A7 | PASS（工具构造）；正式产物BLOCKED | cases：64项纯几何构造，16裁切含4贴边对照，7缺片可见真值清空，3普通块、6桥多边形、六候选、镜像/共线、全部资源与范围外枚举；独立方块/白桥检查连通成功与失败 | 未获批准不渲染MARK正式负例；正式桥实际连通/白片实际域数检查将在正式generate/verify运行，未声称算法拒绝或资源截断 |
| A8 | PASS（selftest）；formal单例BLOCKED | acceptance-final-a/b 真值文件hash同为 a0fcb4124dfd338be004338cbcc2cf831202852494610d94f0d3f61fc466bf3f；四张PNG逐一相同；docs/evidence/repeat-hashes.log；各manifest记录环境 | 同环境重建全部真值和PNG；正式单例重复因无批准未执行 |
| A9 | PASS（工具） | 两个完整selftest verify通过；workflow删PNG后失败并给唯一case_id；损坏PNG hash、INCOMPLETE均拒绝；单例subset=true且数量1独立通过 | subset不冒充完整网格；故障副本留在证据记录的/tmp目录 |
| A10 | PASS（人工与源码检查） | 自编实现仅已知几何/偏移/栅格/文件检查；未调用检测、分类、轮廓提取/拟合、仿射估计、解码、阈值搜索或视频API；仅工具目录和/tmp写入 | cases.baseline字段指spec定义的构造基准，未导入作业baseline；无检测容差配置 |
| A11 | PASS（独立流程） | 全新build-acceptance按README相同流程配置/Release编译/CTest，随后validate→plan→selftest→verify；docs/evidence/clean-*.log 与命令日志 | README提供安装、所有命令、批准前提、输出位置和完整排错表；正式操作须有效批准 |
| A12 | PASS（人工逐模块） | 已审阅include、全部9个cpp、tests与CMake：各函数用途、模型校验、已知变换、绕向偏移、覆盖采样/阈值、真值/观测分离、事务/流式hash和独立重建均有中文注释 | 未以注释行数代替阅读；协议helper与异常构造也说明作用 |
| A13 | PASS（工具记录） | 本文与README实际环境、命令、结果、复现方法、耗时与未批准项；算法记录NOT_RUN | 工具验收不是Block 2/Step 4完成 |

## 实际环境与最终命令

GNU 13.3.0；CMake 3.28.3；C++17；Release；OpenCV 4.6.0；nlohmann_json 3.11.3；OpenSSL 3.0.13。环境与最终代码摘要见 `docs/evidence/validate.json` 及最终run manifest。原件hash见sources，配置联合hash为 `18f8a8e22322ce3a3d66b144918c91c8c81752c2c613357ccddec77ec2de9d50`。

已运行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
cmake -S . -B build-acceptance -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-acceptance --parallel 2
ctest --test-dir build-acceptance --output-on-failure
./build-acceptance/synthetic_gen --help
./build-acceptance/synthetic_gen validate
./build-acceptance/synthetic_gen plan --run-id acceptance-clean-plan
./build-acceptance/synthetic_gen generate --mode selftest --run-id acceptance-clean-selftest
./build-acceptance/synthetic_gen verify --run-id acceptance-clean-selftest
./build/synthetic_gen validate
./build/synthetic_gen plan --run-id acceptance-final-plan
./build/synthetic_gen generate --mode selftest --run-id acceptance-final-a
./build/synthetic_gen generate --mode selftest --run-id acceptance-final-b
./build/synthetic_gen verify --run-id acceptance-final-a
./build/synthetic_gen verify --run-id acceptance-final-b
./build/synthetic_gen generate --mode formal --run-id approval-blocked-final
```

前述构建/自测/验证命令成功；最后formal命令预期且实际退出3，未创建run。故障测试的错误hash批准、删图和损坏PNG在/tmp隔离副本，证据JSON给实际路径；不会损坏最终交付的两个selftest目录。旧 acceptance-selftest-* 与 acceptance-plan-20261004 是开发中间版本快照，不作为最终验收，保留以遵守不覆盖已有run规则；最终证据以 acceptance-final-* 和 acceptance-clean-* 为准。

最终规划图像平面保守未压缩上界107,230,003,200字节（约99.87GiB），不含JSON/容器/文件系统。实际selftest每run 5项：4图像、1 PERTURBATION_INVALID；images/masks/instance_masks各4个PNG，observation模板仅列头。完整formal未运行，正式无效样例数量尚未知。

待后续：有效模型与实验批准记录→完整formal与verify→正式单例重复；之后由真实算法填写观测模板和误差/拒绝/多解/资源计数，再走参数确认。工具不会替用户确认这些关卡。
